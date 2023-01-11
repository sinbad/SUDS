#include "SUDSDialogue.h"

#include "SUDSParticipant.h"
#include "SUDSScript.h"
#include "SUDSScriptNode.h"
#include "SUDSScriptNodeEvent.h"
#include "SUDSScriptNodeGosub.h"
#include "SUDSScriptNodeSet.h"
#include "SUDSScriptNodeText.h"

DEFINE_LOG_CATEGORY(LogSUDSDialogue);

const FText USUDSDialogue::DummyText = FText::FromString("INVALID");
const FString USUDSDialogue::DummyString = "INVALID";


FArchive& operator<<(FArchive& Ar, FSUDSDialogueState& Value)
{
	Ar << Value.TextNodeID;
	Ar << Value.Variables;
	Ar << Value.ChoicesTaken;
	Ar << Value.ReturnStack;
	
	return Ar;
}

void operator<<(FStructuredArchive::FSlot Slot, FSUDSDialogueState& Value)
{
	FStructuredArchive::FRecord Record = Slot.EnterRecord();
	Record
		<< SA_VALUE(TEXT("TextNodeID"), Value.TextNodeID)
		<< SA_VALUE(TEXT("Variables"), Value.Variables)
		<< SA_VALUE(TEXT("ChoicesTaken"), Value.ChoicesTaken)
		<< SA_VALUE(TEXT("ReturnStack"), Value.ReturnStack);

}

USUDSDialogue::USUDSDialogue()
{
}

void USUDSDialogue::Initialise(const USUDSScript* Script)
{
	BaseScript = Script;
	CurrentSpeakerNode = nullptr;

	InitVariables();

	CurrentSpeakerNode = nullptr;

}

void USUDSDialogue::InitVariables()
{
	VariableState.Empty();
	// Run header nodes immediately (only set nodes)
	RunUntilNextSpeakerNodeOrEnd(BaseScript->GetHeaderNode(), false);
}

void USUDSDialogue::Start(FName Label)
{
	// Only start if not already on a speaker node
	// This makes the restore sequence easier, you don't have to test IsEnded
	if (!IsValid(CurrentSpeakerNode))
	{
		// Note that we don't reset state by default here. This is to allow long-term memory on dialogue, such as
		// knowing whether you've met a character before etc.
		// We also don't re-run headers here since they will have been run on Initialise()
		// This is to allow callers to set variables before Start() that override headers
		Restart(false, Label, false);
	}
}

void USUDSDialogue::SetParticipants(const TArray<UObject*>& InParticipants)
{
	Participants = InParticipants;
	SortParticipants();
}

void USUDSDialogue::AddParticipant(UObject* Participant)
{
	Participants.Add(Participant);
	SortParticipants();
}

void USUDSDialogue::SortParticipants()
{
	if (!Participants.IsEmpty())
	{
		// We order by ascending priority so that higher priority values are later in the list
		// Which means they're called last and get to override values set by earlier ones
		// We'll do a stable sort so that otherwise order is maintained
		Participants.StableSort([](const UObject& A, const UObject& B)
		{
			return ISUDSParticipant::Execute_GetDialogueParticipantPriority(&A) <
				ISUDSParticipant::Execute_GetDialogueParticipantPriority(&B);
		});
	}
}

void USUDSDialogue::RunUntilNextSpeakerNodeOrEnd(USUDSScriptNode* NextNode, bool bRaiseAtEnd)
{
	// We run through nodes which don't require a speaker line prompt
	// E.g. set nodes, select nodes which are all automatically resolved
	// Starting with this node
	while (NextNode && !IsChoiceOrTextNode(NextNode->GetNodeType()))
	{
		NextNode = RunNode(NextNode);
	}

	if (NextNode)
	{
		if (NextNode->GetNodeType() == ESUDSScriptNodeType::Text)
		{
			SetCurrentSpeakerNode(Cast<USUDSScriptNodeText>(NextNode), false);
		}
		else
		{
			// This can happen if for example user creates a choice node as the first thing
			UE_LOG(LogSUDSDialogue,
			       Error,
			       TEXT("Error in %s line %d: Tried to run to next speaker node but encountered unexpected node of type %s"),
			       *BaseScript->GetName(),
			       NextNode->GetSourceLineNo(),
			       *(StaticEnum<ESUDSScriptNodeType>()->GetValueAsString(NextNode->GetNodeType()))
			);
		}
	}
	else
	{
		End(!bRaiseAtEnd);
	}

}

USUDSScriptNode* USUDSDialogue::RunNode(USUDSScriptNode* Node)
{
	switch (Node->GetNodeType())
	{
	case ESUDSScriptNodeType::Select:
		return RunSelectNode(Node);
	case ESUDSScriptNodeType::SetVariable:
		return RunSetVariableNode(Node);
	case ESUDSScriptNodeType::Event:
		return RunEventNode(Node);
	case ESUDSScriptNodeType::Gosub:
		return RunGosubNode(Node);
	case ESUDSScriptNodeType::Return:
		return RunReturnNode(Node);
	default: ;
	}

	UE_LOG(LogSUDSDialogue,
	       Error,
	       TEXT("Error in %s line %d: Attempted to run non-runnable node type %s"),
	       *BaseScript->GetName(),
	       Node->GetSourceLineNo(),
	       *(StaticEnum<ESUDSScriptNodeType>()->GetValueAsString(Node->GetNodeType()))
	)
	return nullptr;
}

USUDSScriptNode* USUDSDialogue::RunSelectNode(USUDSScriptNode* Node)
{
	for (auto& Edge : Node->GetEdges())
	{
		if (Edge.GetCondition().IsValid())
		{
			// use the first satisfied edge
			RaiseExpressionVariablesRequested(Edge.GetCondition());
			if (Edge.GetCondition().EvaluateBoolean(VariableState, BaseScript->GetName()))
			{
				return Edge.GetTargetNode().Get();
			}
		}
	}
	// NOTE: if no valid path, go to end
	// We've already created fall-through else nodes if possible
	return nullptr;
}

USUDSScriptNode* USUDSDialogue::RunEventNode(USUDSScriptNode* Node)
{
	if (USUDSScriptNodeEvent* EvtNode = Cast<USUDSScriptNodeEvent>(Node))
	{
		// Build a resolved args list, because we need to evaluate  expressions
		TArray<FSUDSValue> ArgsResolved;
		
		for (auto& Expr : EvtNode->GetArgs())
		{
			RaiseExpressionVariablesRequested(Expr);
			ArgsResolved.Add(Expr.Evaluate(VariableState));
		}
		
		for (const auto P : Participants)
		{
			if (P->GetClass()->ImplementsInterface(USUDSParticipant::StaticClass()))
			{
				ISUDSParticipant::Execute_OnDialogueEvent(P, this, EvtNode->GetEventName(), ArgsResolved);
			}
		}
		OnEvent.Broadcast(this, EvtNode->GetEventName(), ArgsResolved);
	}
	return GetNextNode(Node);
}

USUDSScriptNode* USUDSDialogue::RunGosubNode(USUDSScriptNode* Node)
{
	if (USUDSScriptNodeGosub* GosubNode = Cast<USUDSScriptNodeGosub>(Node))
	{
		if (auto TargetNode = BaseScript->GetNodeByLabel(GosubNode->GetLabelName()))
		{
			// Push this gosub node to the return stack, then jump
			GosubReturnStack.Push(GosubNode);
			return TargetNode;
		}
		else
		{
			UE_LOG(LogSUDSDialogue,
				   Error,
				   TEXT("Error in %s: Cannot gosub to label '%s', was not found"),
				   *BaseScript->GetName(),
				   *GosubNode->GetLabelName().ToString());
			
		}
	}
	return GetNextNode(Node);
}

USUDSScriptNode* USUDSDialogue::RunReturnNode(USUDSScriptNode* Node)
{
	if (GosubReturnStack.Num() > 0)
	{
		// We return to the next node after the gosub, which temporarily redirected
		const auto GoSubNode = GosubReturnStack.Pop();
		return GetNextNode(GoSubNode);
	}
	else
	{
		UE_LOG(LogSUDSDialogue,
			   Error,
			   TEXT("Attempted to return at %s:%d but there was no previous gosub to return to"),
			   *BaseScript->GetName(),
			   Node->GetSourceLineNo());
		return nullptr;
		
	}
}

USUDSScriptNode* USUDSDialogue::RunSetVariableNode(USUDSScriptNode* Node)
{
	if (USUDSScriptNodeSet* SetNode = Cast<USUDSScriptNodeSet>(Node))
	{
		if (SetNode->GetExpression().IsValid())
		{
			RaiseExpressionVariablesRequested(SetNode->GetExpression());
			const FSUDSValue OldValue = GetVariable(SetNode->GetIdentifier());
			const FSUDSValue NewValue = SetNode->GetExpression().Evaluate(VariableState);
			if ((OldValue != NewValue).GetBooleanValue())
			{
				VariableState.Add(SetNode->GetIdentifier(), NewValue);
				RaiseVariableChange(SetNode->GetIdentifier(), NewValue, true);
			}
		}
	}

	// Always one edge
	return GetNextNode(Node);
	
}

void USUDSDialogue::RaiseVariableChange(const FName& VarName, const FSUDSValue& Value, bool bFromScript)
{
	for (const auto P : Participants)
	{
		if (P->GetClass()->ImplementsInterface(USUDSParticipant::StaticClass()))
		{
			ISUDSParticipant::Execute_OnDialogueVariableChanged(P, this, VarName, Value, bFromScript);
		}
	}
	OnVariableChanged.Broadcast(this, VarName, Value, bFromScript);

}

void USUDSDialogue::RaiseVariableRequested(const FName& VarName)
{
	// Because variables set by participants should "win", raise event first
	OnVariableRequested.Broadcast(this, VarName);
	for (const auto P : Participants)
	{
		if (P->GetClass()->ImplementsInterface(USUDSParticipant::StaticClass()))
		{
			ISUDSParticipant::Execute_OnDialogueVariableRequested(P, this, VarName);
		}
	}
}

void USUDSDialogue::RaiseExpressionVariablesRequested(const FSUDSExpression& Expression)
{
	for (auto& Var : Expression.GetVariableNames())
	{
		RaiseVariableRequested(Var);
	}
}

void USUDSDialogue::SetCurrentSpeakerNode(USUDSScriptNodeText* Node, bool bQuietly)
{
	CurrentSpeakerNode = Node;

	CurrentSpeakerDisplayName = FText::GetEmpty();
	bParamNamesExtracted = false;

	UpdateChoices();

	if (!bQuietly)
	{
		if (CurrentSpeakerNode)
			RaiseNewSpeakerLine();
		else
			RaiseFinished();
	}

}

FText USUDSDialogue::ResolveParameterisedText(const TArray<FName> Params, const FTextFormat& TextFormat)
{
	for (const auto& P : Params)
	{
		RaiseVariableRequested(P);
	}
	// Need to make a temp arg list for compatibility
	// Also lets us just set the ones we need to
	FFormatNamedArguments Args;
	GetTextFormatArgs(Params, Args);
	return FText::Format(TextFormat, Args);
	
}

void USUDSDialogue::GetTextFormatArgs(const TArray<FName>& ArgNames, FFormatNamedArguments& OutArgs) const
{
	for (auto& Name : ArgNames)
	{
		if (const FSUDSValue* Value = VariableState.Find(Name))
		{
			// Use the operator conversion
			OutArgs.Add(Name.ToString(), Value->ToFormatArg());
		}
	}
}

FText USUDSDialogue::GetText()
{
	if (CurrentSpeakerNode)
	{
		if (CurrentSpeakerNode->HasParameters())
		{
			return ResolveParameterisedText(CurrentSpeakerNode->GetParameterNames(), CurrentSpeakerNode->GetTextFormat());
		}
		else
		{
			return CurrentSpeakerNode->GetText();
		}
	}
	return DummyText;
}

const FString& USUDSDialogue::GetSpeakerID() const
{
	if (CurrentSpeakerNode)
		return CurrentSpeakerNode->GetSpeakerID();
	
	return DummyString;
}

FText USUDSDialogue::GetSpeakerDisplayName() const
{
	if (CurrentSpeakerDisplayName.IsEmpty())
	{
		// Derive speaker display name
		// Is just a special variable "SpeakerName.SpeakerID"
		// or just the SpeakerID if none specified
		static const FString SpeakerIDPrefix = "SpeakerName.";
		FName Key(SpeakerIDPrefix + GetSpeakerID());
		if (auto Arg = VariableState.Find(Key))
		{
			if (Arg->GetType() == ESUDSValueType::Text)
			{
				CurrentSpeakerDisplayName = Arg->GetTextValue();
			}
			else
			{
				UE_LOG(LogSUDSDialogue,
				       Error,
				       TEXT("Error in %s: %s was set to a value that was not text, cannot use"),
				       *BaseScript->GetName(),
				       *Key.ToString());
			}
		}
		if (CurrentSpeakerDisplayName.IsEmpty())
		{
			// If no display name was specified, use the (non-localised) speaker ID
			CurrentSpeakerDisplayName = FText::FromString(GetSpeakerID());
		}
	}
	return CurrentSpeakerDisplayName;
}

USUDSScriptNode* USUDSDialogue::GetNextNode(USUDSScriptNode* Node)
{
	// In the case of select, we need to evaluate to get the next node
	if (Node->GetNodeType() == ESUDSScriptNodeType::Select)
	{
		return RunSelectNode(Node);	
	}
	else
	{
		return BaseScript->GetNextNode(Node);
	}
}

bool USUDSDialogue::IsChoiceOrTextNode(ESUDSScriptNodeType Type)
{
	return Type == ESUDSScriptNodeType::Text || Type == ESUDSScriptNodeType::Choice;
}

const USUDSScriptNode* USUDSDialogue::WalkToNextChoiceNode(USUDSScriptNode* FromNode, bool bExecute)
{
	if (FromNode && FromNode->GetEdgeCount() == 1)
	{
		const auto NextNode = GetNextNode(FromNode);
		const auto ResultNode = RecurseWalkToNextChoiceOrTextNode(NextNode, bExecute, 0);
		if (ResultNode && ResultNode->GetNodeType() == ESUDSScriptNodeType::Choice)
		{
			return ResultNode;
		}
	}
	return nullptr;
}

const USUDSScriptNode* USUDSDialogue::RecurseWalkToNextChoiceOrTextNode(USUDSScriptNode* Node, bool bExecute, int GosubReturnSearchDepth)
{
	auto NextNode = Node;
	while (NextNode && !IsChoiceOrTextNode(NextNode->GetNodeType()))
	{
		// Special case gosub/return in non-execute mode, since only RunNode will explore them
		if (!bExecute)
		{
			if (NextNode->GetNodeType() == ESUDSScriptNodeType::Gosub)
			{
				// We need to special case Gosubs, since to find the choice we have to go into them and potentially out again
				if (const USUDSScriptNodeGosub* GosubNode = Cast<USUDSScriptNodeGosub>(NextNode))
				{
					if (auto SubNode = BaseScript->GetNodeByLabel(GosubNode->GetLabelName()))
					{
						if (auto Result = RecurseWalkToNextChoiceOrTextNode(SubNode, bExecute, GosubReturnSearchDepth + 1))
						{
							// We hit a choice or text inside the sub
							return Result;
						}
						// We exited the sub without finding anything, so continue after
						// Fall through to GetNextNode below
					}
				}
						
			}
			else if (NextNode->GetNodeType() == ESUDSScriptNodeType::Return)
			{
				// A return can occur in 2 scenarios:
				// 1. We recursed here, above, in which case we can just return to come back out
				// 2. The current dialogue state is inside a gosub, in which case we have to use the gosub stack

				if (GosubReturnSearchDepth > 0)
				{
					// This was exploratory recursion in this function, so just return as not finding
					return nullptr;
				}
				else
				{
					// If we're at depth 0 then we must be returning from a current dialogue state inside a gosub
					// The GosubReturnSearchDepth goes negative if we return multiple times from this site (nested gosub)
					if (GosubReturnStack.Num() > -GosubReturnSearchDepth)
					{
						// We try to find the next choice node after the gosub, which temporarily redirected
						const auto GoSubNode = GosubReturnStack.Last(-GosubReturnSearchDepth);
						return RecurseWalkToNextChoiceOrTextNode(GetNextNode(GoSubNode), bExecute, GosubReturnSearchDepth - 1);
					}
				}
			}
		}
		
		if (bExecute)
		{
			NextNode = RunNode(NextNode);
		}
		else
		{
			NextNode = GetNextNode(NextNode);
		}
	}

	return NextNode;
}

const USUDSScriptNode* USUDSDialogue::RunUntilNextChoiceNode(USUDSScriptNode* FromNode)
{
	return WalkToNextChoiceNode(FromNode, true);
}
const USUDSScriptNode* USUDSDialogue::FindNextChoiceNode(USUDSScriptNode* FromNode)
{
	return WalkToNextChoiceNode(FromNode, false);
}

const TArray<FSUDSScriptEdge>& USUDSDialogue::GetChoices() const
{
	return CurrentChoices;
}

void USUDSDialogue::RecurseAppendChoices(const USUDSScriptNode* Node, TArray<FSUDSScriptEdge>& OutChoices)
{
	if (!Node)
		return;

	// We only cascade into choices or selects
	if(Node->GetNodeType() != ESUDSScriptNodeType::Choice &&
		Node->GetNodeType() != ESUDSScriptNodeType::Select)
	{
		return;
	}
	
	for (auto& Edge : Node->GetEdges())
	{
		switch (Edge.GetType())
		{
		case ESUDSEdgeType::Decision:
			OutChoices.Add(Edge);
			break;
		case ESUDSEdgeType::Condition:
			// Conditional edges are under selects
			if (Edge.GetCondition().IsValid())
			{
				RaiseExpressionVariablesRequested(Edge.GetCondition());
				if (Edge.GetCondition().EvaluateBoolean(VariableState, BaseScript->GetName()))
				{
					RecurseAppendChoices(Edge.GetTargetNode().Get(), OutChoices);
					// When we choose a path on a select, we don't check the other paths, we can only go down one
					return;
				}
			}
			break;
		case ESUDSEdgeType::Chained:
			RecurseAppendChoices(Edge.GetTargetNode().Get(), OutChoices);
			break;
		default:
		case ESUDSEdgeType::Continue:
			UE_LOG(LogSUDSDialogue, Fatal, TEXT("Should not have encountered invalid edge in RecurseAppendChoices"))			
			break;
		};
		
	}
}

void USUDSDialogue::UpdateChoices()
{
	CurrentChoices.Reset();
	if (CurrentSpeakerNode)
	{
		// If we've either found choices through static checking (on one or other select paths), we look for them now
		// We also check if we're inside a gosub, since the call site changes whether there may be choices or not
		if (CurrentSpeakerNode->MayHaveChoices() ||
			GosubReturnStack.Num() > 0)
		{
			// We MIGHT have a choice; conditionals can result in HasChoices() being true but the current state not actually
			// taking us to a choice path
			if (const USUDSScriptNode* ChoiceNode = FindNextChoiceNode(CurrentSpeakerNode))
			{
				// Run any e.g. set nodes between text and choice
				// These can be set nodes directly under the text and before the first choice, which get run for all choices
				RunUntilNextChoiceNode(CurrentSpeakerNode);

				// Once we've found & run up to the root choice, there can be potentially a tree of mixed choice/select nodes
				// for supporting conditional choices
				RecurseAppendChoices(ChoiceNode, CurrentChoices);
			}
		}

		if (CurrentChoices.Num() == 0)
		{
			if (auto Edge = CurrentSpeakerNode->GetEdge(0))
			{
				// Simple no-choice progression
				// May occur if HasChoices was true but in current state no choice was found
				CurrentChoices.Add(*Edge);
			}			
		}
	}
}


int USUDSDialogue::GetNumberOfChoices() const
{
	return CurrentChoices.Num();
}

FText USUDSDialogue::GetChoiceText(int Index)
{

	if (CurrentChoices.IsValidIndex(Index))
	{
		auto& Choice = CurrentChoices[Index];
		if (Choice.HasParameters())
		{
			return ResolveParameterisedText(Choice.GetParameterNames(), Choice.GetTextFormat());
		}
		else
		{
			return Choice.GetText();
		}
	}
	else
	{
		UE_LOG(LogSUDSDialogue, Error, TEXT("Invalid choice index %d on node %s"), Index, *GetText().ToString());
	}

	return DummyText;
}

bool USUDSDialogue::HasChoiceIndexBeenTakenPreviously(int Index)
{
	if (CurrentChoices.IsValidIndex(Index))
	{
		return HasChoiceBeenTakenPreviously(CurrentChoices[Index]);
	}
	return false;
}

bool USUDSDialogue::HasChoiceBeenTakenPreviously(const FSUDSScriptEdge& Choice)
{
	return ChoicesTaken.Contains(Choice.GetTextID());
}

bool USUDSDialogue::Continue()
{
	if (GetNumberOfChoices() == 1)
	{
		return Choose(0);		
	}
	return !IsEnded();
}

bool USUDSDialogue::Choose(int Index)
{
	if (CurrentChoices.IsValidIndex(Index))
	{
		// ONLY run to choice node if there is one!
		// This method is called for Continue() too, which has no choice node
		if (CurrentNodeHasChoices())
		{
			ChoicesTaken.Add(CurrentChoices[Index].GetTextID());
			
			RaiseChoiceMade(Index);
			RaiseProceeding();
		}
		else
		{
			RaiseProceeding();
		}
		// Then choose path
		RunUntilNextSpeakerNodeOrEnd(CurrentChoices[Index].GetTargetNode().Get(), true);
		return !IsEnded();
	}
	else
	{
		UE_LOG(LogSUDSDialogue, Error, TEXT("Invalid choice index %d on node %s"), Index, *GetText().ToString());
	}
	return false;
}

bool USUDSDialogue::CurrentNodeHasChoices() const
{
	if (!CurrentSpeakerNode)
		return false;

	// early-out precomputed text node choice indicator
	if (CurrentSpeakerNode->MayHaveChoices())
		return true;

	// Alternatively, if the next node is a return, the site of the gosub determines whether there are choices
	if (auto Edge = CurrentSpeakerNode->GetEdge(0))
	{
		const auto Target = Edge->GetTargetNode();
		if (Target.IsValid() && Target->GetNodeType() == ESUDSScriptNodeType::Return)
		{
			// Returning from a gosub *might* go back to a choice, we can't know ahead of time, it depends on context
			if (GosubReturnStack.Num() > 0)
			{
				// We try to find the next choice node after the gosub, which temporarily redirected
				const auto GoSubNode = GosubReturnStack.Top();
				return GoSubNode->MayHaveChoices();
			}
		}
	}

	return false;

}

bool USUDSDialogue::IsEnded() const
{
	return CurrentSpeakerNode == nullptr;
}

void USUDSDialogue::End(bool bQuietly)
{
	SetCurrentSpeakerNode(nullptr, bQuietly);
}

void USUDSDialogue::ResetState(bool bResetVariables, bool bResetPosition, bool bResetVisited)
{
	if (bResetVariables)
		InitVariables();
	if (bResetPosition)
		SetCurrentSpeakerNode(nullptr, true);
	if (bResetVisited)
		ChoicesTaken.Reset();
}

FSUDSDialogueState USUDSDialogue::GetSavedState() const
{
	const FString CurrentNodeId = CurrentSpeakerNode
		                              ? FTextInspector::GetTextId(CurrentSpeakerNode->GetText()).GetKey().GetChars()
		                              : FString();

	TArray<FString> ExportReturnStack;
	for (auto Node : GosubReturnStack)
	{
		if (auto GN = Cast<USUDSScriptNodeGosub>(Node))
		{
			ExportReturnStack.Add(GN->GetGosubID());
		}
		
	}
	return FSUDSDialogueState(CurrentNodeId, VariableState, ChoicesTaken, ExportReturnStack);
		  
}

void USUDSDialogue::RestoreSavedState(const FSUDSDialogueState& State)
{
	// Don't just empty variables
	// Re-run init to ensure header state is initialised then merge; important for it script is altered since state saved
	InitVariables();
	VariableState.Append(State.GetVariables());
	ChoicesTaken.Empty();
	ChoicesTaken.Append(State.GetChoicesTaken());
	GosubReturnStack.Empty();
	for (auto ID : State.GetReturnStack())
	{
		USUDSScriptNodeGosub* Node = BaseScript->GetNodeByGosubID(ID);
		if (!Node)
		{
			UE_LOG(LogSUDSDialogue, Error, TEXT("Restore: Can't find Gosub with ID %s, returns referencing it will go to end"))
		}
		// Add anyway, will just go to end
		GosubReturnStack.Add(Node);
	}
	
	// If not found this will be null
	if (!State.GetTextNodeID().IsEmpty())
	{
		USUDSScriptNodeText* Node = BaseScript->GetNodeByTextID(State.GetTextNodeID());
		SetCurrentSpeakerNode(Node, true);
	}
	else
	{
		SetCurrentSpeakerNode(nullptr, true);
	}
}

void USUDSDialogue::Restart(bool bResetState, FName StartLabel, bool bReRunHeader)
{
	if (bResetState)
	{
		ResetState();
	}
	// Always reset return stack
	GosubReturnStack.Empty();

	RaiseStarting(StartLabel);

	if (!bResetState && bReRunHeader)
	{
		// Run header nodes but don't re-init
		RunUntilNextSpeakerNodeOrEnd(BaseScript->GetHeaderNode(), false);
	}

	if (StartLabel != NAME_None)
	{
		// Check that StartLabel leads to a text node
		// Labels can lead to choices or select nodes for looping, but there has to be a text node to start with.
		auto StartNode = BaseScript->GetNodeByLabel(StartLabel);
		if (!StartNode)
		{
			UE_LOG(LogSUDSDialogue, Error, TEXT("No start label called %s in dialogue %s"), *StartLabel.ToString(), *BaseScript->GetName());
			StartNode = BaseScript->GetFirstNode();
		}
		else if (StartNode->GetNodeType() == ESUDSScriptNodeType::Choice)
		{
			UE_LOG(LogSUDSDialogue,
			       Error,
			       TEXT("Label %s in dialogue %s cannot be used as a start point, points to a choice."),
			       *StartLabel.ToString(),
			       *BaseScript->GetName());
			StartNode = BaseScript->GetFirstNode();
		}
		RunUntilNextSpeakerNodeOrEnd(StartNode, true);
	}
	else
	{
		RunUntilNextSpeakerNodeOrEnd(BaseScript->GetFirstNode(), true);
	}
	
}


TSet<FName> USUDSDialogue::GetParametersInUse()
{
	// Build on demand, may not be needed
	if (!bParamNamesExtracted)
	{
		CurrentRequestedParamNames.Reset();
		if (CurrentSpeakerNode && CurrentSpeakerNode->HasParameters())
		{
			CurrentRequestedParamNames.Append(CurrentSpeakerNode->GetParameterNames());
		}
		for (auto& Choice : CurrentChoices)
		{
			if (Choice.HasParameters())
			{
				CurrentRequestedParamNames.Append(Choice.GetParameterNames());
			}
		}
		bParamNamesExtracted = true;
	}

	return CurrentRequestedParamNames;
	
}

void USUDSDialogue::RaiseStarting(FName StartLabel)
{
	for (const auto P : Participants)
	{
		if (P->GetClass()->ImplementsInterface(USUDSParticipant::StaticClass()))
		{
			ISUDSParticipant::Execute_OnDialogueStarting(P, this, StartLabel);
		}
	}
	OnStarting.Broadcast(this, StartLabel);
}

void USUDSDialogue::RaiseFinished()
{
	for (const auto P : Participants)
	{
		if (P->GetClass()->ImplementsInterface(USUDSParticipant::StaticClass()))
		{
			ISUDSParticipant::Execute_OnDialogueFinished(P, this);
		}
	}
	OnFinished.Broadcast(this);

}

void USUDSDialogue::RaiseNewSpeakerLine()
{
	for (const auto P : Participants)
	{
		if (P->GetClass()->ImplementsInterface(USUDSParticipant::StaticClass()))
		{
			ISUDSParticipant::Execute_OnDialogueSpeakerLine(P, this);
		}
	}
	
	// Event listeners get it after
	OnSpeakerLine.Broadcast(this);
}

void USUDSDialogue::RaiseChoiceMade(int Index)
{
	for (const auto P : Participants)
	{
		if (P->GetClass()->ImplementsInterface(USUDSParticipant::StaticClass()))
		{
			ISUDSParticipant::Execute_OnDialogueChoiceMade(P, this, Index);
		}
	}
	// Event listeners get it after
	OnChoice.Broadcast(this, Index);
}

void USUDSDialogue::RaiseProceeding()
{
	for (const auto P : Participants)
	{
		if (P->GetClass()->ImplementsInterface(USUDSParticipant::StaticClass()))
		{
			ISUDSParticipant::Execute_OnDialogueProceeding(P, this);
		}
	}
	// Event listeners get it after
	OnProceeding.Broadcast(this);
}

FText USUDSDialogue::GetVariableText(FName Name)
{
	if (auto Arg = VariableState.Find(Name))
	{
		if (Arg->GetType() == ESUDSValueType::Text)
		{
			return Arg->GetTextValue();
		}
		else
		{
			UE_LOG(LogSUDSDialogue, Error, TEXT("Requested variable %s of type text but was not a compatible type"), *Name.ToString());
		}
	}
	return FText();
}

void USUDSDialogue::SetVariableInt(FName Name, int32 Value)
{
	SetVariable(Name, Value);
}

int USUDSDialogue::GetVariableInt(FName Name)
{
	if (auto Arg = VariableState.Find(Name))
	{
		switch (Arg->GetType())
		{
		case ESUDSValueType::Int:
			return Arg->GetIntValue();
		case ESUDSValueType::Float:
			UE_LOG(LogSUDSDialogue, Warning, TEXT("Casting variable %s to int, data loss may occur"), *Name.ToString());
			return Arg->GetFloatValue();
		default: 
		case ESUDSValueType::Gender:
		case ESUDSValueType::Text:
			UE_LOG(LogSUDSDialogue, Error, TEXT("Variable %s is not a compatible integer type"), *Name.ToString());
		}
	}
	return 0;
}

void USUDSDialogue::SetVariableFloat(FName Name, float Value)
{
	SetVariable(Name, Value);
}

float USUDSDialogue::GetVariableFloat(FName Name)
{
	if (auto Arg = VariableState.Find(Name))
	{
		switch (Arg->GetType())
		{
		case ESUDSValueType::Int:
			return Arg->GetIntValue();
		case ESUDSValueType::Float:
			return Arg->GetFloatValue();
		default: 
		case ESUDSValueType::Gender:
		case ESUDSValueType::Text:
			UE_LOG(LogSUDSDialogue, Error, TEXT("Variable %s is not a compatible float type"), *Name.ToString());
		}
	}
	return 0;
}

void USUDSDialogue::SetVariableGender(FName Name, ETextGender Value)
{
	SetVariable(Name, Value);
}

ETextGender USUDSDialogue::GetVariableGender(FName Name)
{
	if (auto Arg = VariableState.Find(Name))
	{
		switch (Arg->GetType())
		{
		case ESUDSValueType::Gender:
			return Arg->GetGenderValue();
		default: 
		case ESUDSValueType::Int:
		case ESUDSValueType::Float:
		case ESUDSValueType::Text:
			UE_LOG(LogSUDSDialogue, Error, TEXT("Variable %s is not a compatible gender type"), *Name.ToString());
		}
	}
	return ETextGender::Neuter;
}

void USUDSDialogue::SetVariableBoolean(FName Name, bool Value)
{
	// Use explicit FSUDSValue constructor to avoid default int conversion
	SetVariable(Name, FSUDSValue(Value));
}

bool USUDSDialogue::GetVariableBoolean(FName Name)
{
	if (auto Arg = VariableState.Find(Name))
	{
		switch (Arg->GetType())
		{
		case ESUDSValueType::Boolean:
			return Arg->GetBooleanValue();
		case ESUDSValueType::Int:
			return Arg->GetIntValue() != 0;
		default: 
		case ESUDSValueType::Float:
		case ESUDSValueType::Gender:
		case ESUDSValueType::Text:
			UE_LOG(LogSUDSDialogue, Error, TEXT("Variable %s is not a compatible boolean type"), *Name.ToString());
		}
	}
	return false;
}

void USUDSDialogue::SetVariableName(FName Name, FName Value)
{
	SetVariable(Name, FSUDSValue(Value, false));
}

FName USUDSDialogue::GetVariableName(FName Name)
{
	if (auto Arg = VariableState.Find(Name))
	{
		if (Arg->GetType() == ESUDSValueType::Name)
		{
			return Arg->GetNameValue();
		}
		else
		{
			UE_LOG(LogSUDSDialogue, Error, TEXT("Requested variable %s of type text but was not a compatible type"), *Name.ToString());
		}
	}
	return NAME_None;
}
