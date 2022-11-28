#include "SUDSDialogue.h"

#include "SUDSParticipant.h"
#include "SUDSScript.h"
#include "SUDSScriptNode.h"
#include "SUDSScriptNodeEvent.h"
#include "SUDSScriptNodeSet.h"
#include "SUDSScriptNodeText.h"

DEFINE_LOG_CATEGORY(LogSUDSDialogue)

PRAGMA_DISABLE_OPTIMIZATION

const FText USUDSDialogue::DummyText = FText::FromString("INVALID");
const FString USUDSDialogue::DummyString = "INVALID";

USUDSDialogue::USUDSDialogue()
{
}

void USUDSDialogue::Initialise(const USUDSScript* Script)
{
	BaseScript = Script;
	CurrentSpeakerNode = nullptr;

	// Run header nodes immediately (only set nodes)
	RunUntilNextSpeakerNodeOrEnd(BaseScript->GetHeaderNode());

	CurrentSpeakerNode = nullptr;

}

void USUDSDialogue::Start(FName Label)
{
	// Note that we don't reset state by default here. This is to allow long-term memory on dialogue, such as
	// knowing whether you've met a character before etc.
	// We also don't re-run headers here since they will have been run on Initialise()
	// This is to allow callers to set variables before Start() that override headers
	Restart(false, Label, false);
}

void USUDSDialogue::SetParticipants(const TArray<UObject*> InParticipants)
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

void USUDSDialogue::RunUntilNextSpeakerNodeOrEnd(USUDSScriptNode* NextNode)
{
	// We run through nodes which don't require a speaker line prompt
	// E.g. set nodes, select nodes which are all automatically resolved
	// Starting with this node
	while (NextNode && !ShouldStopAtNodeType(NextNode->GetNodeType()))
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
		// Reached the end
		SetCurrentSpeakerNode(nullptr, true);
		OnFinished.Broadcast(this);
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

USUDSScriptNode* USUDSDialogue::RunSelectNode(USUDSScriptNode* Node) const
{
	for (auto& Edge : Node->GetEdges())
	{
		// use the first satisfied edge
		if (Edge.GetCondition().EvaluateBoolean(VariableState, BaseScript->GetName()))
		{
			return Edge.GetTargetNode().Get();
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

USUDSScriptNode* USUDSDialogue::RunSetVariableNode(USUDSScriptNode* Node)
{
	if (USUDSScriptNodeSet* SetNode = Cast<USUDSScriptNodeSet>(Node))
	{
		if (SetNode->GetExpression().IsValid())
		{
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

void USUDSDialogue::SetCurrentSpeakerNode(USUDSScriptNodeText* Node, bool bQuietly)
{
	CurrentSpeakerNode = Node;

	CurrentSpeakerDisplayName = FText::GetEmpty();
	bParamNamesExtracted = false;

	UpdateChoices();

	if (!bQuietly)
		RaiseNewSpeakerLine();

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

FText USUDSDialogue::GetText() const
{
	if (CurrentSpeakerNode)
	{
		if (CurrentSpeakerNode->HasParameters())
		{
			// Need to make a temp arg list for compatibility
			// Also lets us just set the ones we need to
			FFormatNamedArguments Args;
			GetTextFormatArgs(CurrentSpeakerNode->GetParameterNames(), Args);
			return FText::Format(CurrentSpeakerNode->GetTextFormat(), Args);
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

USUDSScriptNode* USUDSDialogue::GetNextNode(const USUDSScriptNode* Node) const
{
	return BaseScript->GetNextNode(Node);
}

bool USUDSDialogue::ShouldStopAtNodeType(ESUDSScriptNodeType Type)
{
	return Type != ESUDSScriptNodeType::SetVariable &&
		Type != ESUDSScriptNodeType::Select &&
		Type != ESUDSScriptNodeType::Event;
}

const USUDSScriptNode* USUDSDialogue::RunUntilNextChoiceNode(const USUDSScriptNodeText* FromTextNode)
{
	if (FromTextNode && FromTextNode->GetEdgeCount() == 1)
	{
		auto NextNode = GetNextNode(FromTextNode);
		// We skip over set nodes
		while (NextNode && !ShouldStopAtNodeType(NextNode->GetNodeType()))
		{
			NextNode = RunNode(NextNode);
		}

		return NextNode;
	}

	return nullptr;
	
}

const TArray<FSUDSScriptEdge>& USUDSDialogue::GetChoices() const
{
	return CurrentChoices;
}

void USUDSDialogue::RecurseAppendChoices(const USUDSScriptNode* Node, TArray<FSUDSScriptEdge>& OutChoices)
{
	if (!Node)
		return;

	check(Node->GetNodeType() == ESUDSScriptNodeType::Choice || Node->GetNodeType() == ESUDSScriptNodeType::Select);
	
	for (auto& Edge : Node->GetEdges())
	{
		switch (Edge.GetType())
		{
		case ESUDSEdgeType::Decision:
			OutChoices.Add(Edge);
			break;
		case ESUDSEdgeType::Condition:
			// Conditional edges are under selects
			if (Edge.GetCondition().EvaluateBoolean(VariableState, BaseScript->GetName()))
			{
				RecurseAppendChoices(Edge.GetTargetNode().Get(), OutChoices);
				// When we choose a path on a select, we don't check the other paths, we can only go down one
				return;
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
		if (CurrentSpeakerNode->HasChoices())
		{
			// Root choice node might not be directly underneath. For example, we may go through set nodes first
			if (const USUDSScriptNode* ChoiceNode = BaseScript->GetNextChoiceNode(CurrentSpeakerNode))
			{
				// Once we've found the root choice, there can be potentially a tree of mixed choice/select nodes
				// for supporting conditional choices
				RecurseAppendChoices(ChoiceNode, CurrentChoices);
			}
		}
		else
		{
			if (auto Edge = CurrentSpeakerNode->GetEdge(0))
			{
				// Simple no-choice progression (text->text)
				CurrentChoices.Add(*Edge);
			}			
		}
	}
}


int USUDSDialogue::GetNumberOfChoices() const
{
	return CurrentChoices.Num();
}

FText USUDSDialogue::GetChoiceText(int Index) const
{

	if (CurrentChoices.IsValidIndex(Index))
	{
		auto& Choice = CurrentChoices[Index];
		if (Choice.HasParameters())
		{
			// Need to make a temp arg list for compatibility
			// Also lets us just set the ones we need to
			FFormatNamedArguments Args;
			GetTextFormatArgs(Choice.GetParameterNames(), Args);
			return FText::Format(Choice.GetTextFormat(), Args);
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

bool USUDSDialogue::HasChoiceBeenTakenPreviously(int Index)
{
	if (CurrentChoices.IsValidIndex(Index))
	{
		return ChoicesTaken.Contains(CurrentChoices[Index].GetTextID());
	}
	return false;
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
		ChoicesTaken.Add(CurrentChoices[Index].GetTextID());
		
		// ONLY run to choice node if there is one!
		// This method is called for Continue() too, which has no choice node
		if (CurrentSpeakerNode->HasChoices())
		{
			RaiseChoiceMade(Index);
			// Run any e.g. set nodes between text and choice
			// These can be set nodes directly under the text and before the first choice, which get run for all choices
			RunUntilNextChoiceNode(CurrentSpeakerNode);
		}
		// Then choose path
		RunUntilNextSpeakerNodeOrEnd(CurrentChoices[Index].GetTargetNode().Get());
		return !IsEnded();
	}
	else
	{
		UE_LOG(LogSUDSDialogue, Error, TEXT("Invalid choice index %d on node %s"), Index, *GetText().ToString());
	}
	return false;
}

bool USUDSDialogue::IsEnded() const
{
	return CurrentSpeakerNode == nullptr;
}

void USUDSDialogue::ResetState(bool bResetVariables, bool bResetPosition, bool bResetVisited)
{
	if (bResetVariables)
		VariableState.Reset();
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
	return FSUDSDialogueState(CurrentNodeId, VariableState, ChoicesTaken);
		  
}

void USUDSDialogue::RestoreSavedState(const FSUDSDialogueState& State)
{
	VariableState.Empty();
	VariableState = State.GetVariables();
	ChoicesTaken.Empty();
	ChoicesTaken.Append(State.GetChoicesTaken());

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

	RaiseStarting(StartLabel);

	if (bReRunHeader)
	{
		// Run header nodes
		RunUntilNextSpeakerNodeOrEnd(BaseScript->GetHeaderNode());
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
		else if (StartNode->GetNodeType() != ESUDSScriptNodeType::Text)
		{
			UE_LOG(LogSUDSDialogue,
			       Error,
			       TEXT("Label %s in dialogue %s cannot be used as a start point, does not point to a text line."),
			       *StartLabel.ToString(),
			       *BaseScript->GetName());
			StartNode = BaseScript->GetFirstNode();
		}
		RunUntilNextSpeakerNodeOrEnd(StartNode);
	}
	else
	{
		RunUntilNextSpeakerNodeOrEnd(BaseScript->GetFirstNode());
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

PRAGMA_ENABLE_OPTIMIZATION
