#include "SUDSDialogue.h"

#include "SUDSParticipant.h"
#include "SUDSScript.h"
#include "SUDSScriptNode.h"
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
}

void USUDSDialogue::Start(FName Label)
{
	// Note that we don't reset state by default here. This is to allow long-term memory on dialogue, such as
	// knowing whether you've met a character before etc.
	Restart(false, Label);
}

void USUDSDialogue::SetParticipants(const TMap<FString, UObject*> InParticipants)
{
	Participants = InParticipants;
	SortParticipants();
}

void USUDSDialogue::AddParticipant(const FString& RoleName, UObject* Participant)
{
	Participants.Add(RoleName, Participant);
	SortParticipants();
}

void USUDSDialogue::SortParticipants()
{
	if (!Participants.IsEmpty())
	{
		// We order by ascending priority so that higher priority values are later in the list
		// Which means they're called last and get to override values set by earlier ones
		Participants.ValueSort([](const UObject& A, const UObject& B)
		{
			return ISUDSParticipant::Execute_GetDialogueParticipantPriority(&A) <
				ISUDSParticipant::Execute_GetDialogueParticipantPriority(&B);
		});
	}
}

UObject* USUDSDialogue::GetParticipant(const FString& RoleName)
{
	if (auto pRet = Participants.Find(RoleName))
	{
		return *pRet;
	}
	return nullptr;
}

void USUDSDialogue::RunUntilNextSpeakerNodeOrEnd(USUDSScriptNode* NextNode)
{
	// We run through nodes which don't require a speaker line prompt
	// E.g. set nodes, select nodes which are all automatically resolved
	// Starting with this node
	while (NextNode && NextNode->GetNodeType() != ESUDSScriptNodeType::Text)
	{
		switch (NextNode->GetNodeType())
		{
		case ESUDSScriptNodeType::Text:
			// Should not have got here, while condition
			break;
		case ESUDSScriptNodeType::Choice:
			UE_LOG(LogSUDSDialogue, Error, TEXT("Error in %s: Choice node encountered but wasn't immediately following a text node"), *BaseScript->GetName());
			NextNode = nullptr;
			break;
		case ESUDSScriptNodeType::Select:
			NextNode = RunSelectNode(NextNode);
			break;
		case ESUDSScriptNodeType::SetVariable:
			NextNode = RunSetVariableNode(NextNode);
			break;
		default: ;
		}
		
	}

	if (NextNode)
	{
		// This should be a given, since while will only exit when non-null in this case, but still
		if (NextNode->GetNodeType() == ESUDSScriptNodeType::Text)
		{
			SetCurrentSpeakerNode(Cast<USUDSScriptNodeText>(NextNode));
		}
	}
	else
	{
		// Reached the end
		SetCurrentSpeakerNode(nullptr);
		OnFinished.Broadcast(this);
	}

}

USUDSScriptNode* USUDSDialogue::RunSelectNode(USUDSScriptNode* Node)
{
	// TODO: implement select
	return GetNextNode(Node);
}

USUDSScriptNode* USUDSDialogue::RunSetVariableNode(USUDSScriptNode* Node)
{
	// TODO: support things other than literals
	if (USUDSScriptNodeSet* SetNode = Cast<USUDSScriptNodeSet>(Node))
	{
		VariableState.Add(SetNode->GetIdentifier(), SetNode->GetLiteral());
	}

	// Always one edge
	return GetNextNode(Node);
	
}

void USUDSDialogue::SetCurrentSpeakerNode(USUDSScriptNodeText* Node)
{
	CurrentSpeakerNode = Node;

	CurrentSpeakerDisplayName = FText::GetEmpty();
	AllCurrentChoices = nullptr;
	ValidCurrentChoices.Reset();
	bParamNamesExtracted = false;

	RaiseNewSpeakerLine();

}

FText USUDSDialogue::GetText() const
{
	if (CurrentSpeakerNode->HasParameters())
	{
		return FText::Format(CurrentSpeakerNode->GetTextFormat(), VariableState);
	}
	else
	{
		return CurrentSpeakerNode->GetText();
	}
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
		// TODO: derive speaker display name
	}
	return CurrentSpeakerDisplayName;
}

USUDSScriptNode* USUDSDialogue::GetNextNode(const USUDSScriptNode* Node) const
{
	return BaseScript->GetNextNode(Node);
}

const USUDSScriptNode* USUDSDialogue::RunUntilNextChoiceNode(const USUDSScriptNodeText* FromTextNode)
{
	if (FromTextNode && FromTextNode->GetEdgeCount() == 1)
	{
		auto NextNode = GetNextNode(FromTextNode);
		// We only skip over set nodes
		while (NextNode &&
			NextNode->GetNodeType() == ESUDSScriptNodeType::SetVariable)
		{
			NextNode = RunSetVariableNode(NextNode);
		}

		return NextNode;
	}

	return nullptr;
	
}

const USUDSScriptNode* USUDSDialogue::FindNextChoiceNode(const USUDSScriptNodeText* FromTextNode) const
{
	if (FromTextNode && FromTextNode->GetEdgeCount() == 1)
	{
		auto NextNode = GetNextNode(FromTextNode);
		// We only skip over set nodes
		while (NextNode &&
			NextNode->GetNodeType() == ESUDSScriptNodeType::SetVariable)
		{
			NextNode = GetNextNode(NextNode);
		}

		return NextNode;
	}

	return nullptr;
	
}

const TArray<FSUDSScriptEdge>* USUDSDialogue::GetChoices(bool bOnlyValidChoices) const
{
	// "CurrentNode" is always a text node in practice
	// other nodes are passed through but not alighted on
	// Cache choices
	if (!AllCurrentChoices)
	{
		ValidCurrentChoices.Reset();
		if (CurrentSpeakerNode)
		{
			if (CurrentSpeakerNode->HasChoices())
			{
				// Choice node might not be directly underneath. For example, we may go through set nodes first
				if (const USUDSScriptNode* ChoiceNode = FindNextChoiceNode(CurrentSpeakerNode))
				{
					/// Choices are the edges under the choice node
					AllCurrentChoices = &(ChoiceNode->GetEdges());

					if (bOnlyValidChoices)
					{
						// Note that we only re-evaluate conditions once per node change, for stability between counts / indexes
						// Even if conditions are dependent on external data we only sample them once when node changes so everything is consistent
						// TODO: evaluate conditions
						for (auto& Choice : *AllCurrentChoices)
						{
							// Copy happens here
							ValidCurrentChoices.Add(Choice);
						}
					}
				}
			}
			else
			{
				AllCurrentChoices = &ValidCurrentChoices;
				if (auto Edge = CurrentSpeakerNode->GetEdge(0))
				{
					// Simple no-choice progression (text->text)
					ValidCurrentChoices.Add(*Edge);
				}
			}
			
		}
	}

	return bOnlyValidChoices ? &ValidCurrentChoices : AllCurrentChoices;
}
int USUDSDialogue::GetNumberOfChoices(bool bOnlyValidChoices) const
{
	if (auto Choices = GetChoices(bOnlyValidChoices))
	{
		return Choices->Num();
	}
	else
	{
		// If not a choice node, then if there's a single edge that's a "choice" too
		if (CurrentSpeakerNode && CurrentSpeakerNode->GetEdgeCount() == 1)
			return 1;
	}
	
	return 0;
}

FText USUDSDialogue::GetChoiceText(int Index,bool bOnlyValidChoices) const
{

	if (const auto Choices = GetChoices(bOnlyValidChoices))
	{
		if (Choices && Choices->IsValidIndex(Index))
		{
			auto& Choice = (*Choices)[Index];
			if (Choice.HasParameters())
			{
				return FText::Format(Choice.GetTextFormat(), VariableState);
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
	}

	return DummyText;
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
	if (const auto Choices = GetChoices(true))
	{
		if (Choices && Choices->IsValidIndex(Index))
		{
			RaiseChoiceMade(Index);
			// Run any e.g. set nodes between text and choice
			// These can be set nodes directly under the text and before the first choice, which get run for all choices
			RunUntilNextChoiceNode(CurrentSpeakerNode);
			// Then choose path
			RunUntilNextSpeakerNodeOrEnd((*Choices)[Index].GetTargetNode().Get());
			return !IsEnded();
		}
		else
		{
			UE_LOG(LogSUDSDialogue, Error, TEXT("Invalid choice index %d on node %s"), Index, *GetText().ToString());
		}
	}
	return false;
}

bool USUDSDialogue::IsEnded() const
{
	return CurrentSpeakerNode == nullptr;
}

void USUDSDialogue::Restart(bool bResetState, FName StartLabel)
{
	if (bResetState)
	{
		VariableState.Reset();
	}

	RaiseStarting(StartLabel);

	// We always run header nodes
	RunUntilNextSpeakerNodeOrEnd(BaseScript->GetHeaderNode());

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


TSet<FString> USUDSDialogue::GetParametersInUse()
{
	// Build on demand, may not be needed
	if (!bParamNamesExtracted)
	{
		CurrentRequestedParamNames.Reset();
		if (CurrentSpeakerNode && CurrentSpeakerNode->HasParameters())
		{
			CurrentRequestedParamNames.Append(CurrentSpeakerNode->GetParameterNames());
		}
		if (const auto Choices = GetChoices(true))
		{
			for (auto& Choice : *Choices)
			{
				if (Choice.HasParameters())
				{
					CurrentRequestedParamNames.Append(Choice.GetParameterNames());
				}
			}
		}
		bParamNamesExtracted = true;
	}

	return CurrentRequestedParamNames;
	
}

void USUDSDialogue::RaiseStarting(FName StartLabel)
{
	for (const auto& Pair : Participants)
	{
		if (Pair.Value->GetClass()->ImplementsInterface(USUDSParticipant::StaticClass()))
		{
			ISUDSParticipant::Execute_OnDialogueStarting(Pair.Value, this, StartLabel);
		}
	}
	OnStarting.Broadcast(this, StartLabel);
}

void USUDSDialogue::RaiseFinished()
{
	for (const auto& Pair : Participants)
	{
		if (Pair.Value->GetClass()->ImplementsInterface(USUDSParticipant::StaticClass()))
		{
			ISUDSParticipant::Execute_OnDialogueFinished(Pair.Value, this);
		}
	}
	OnFinished.Broadcast(this);

}

void USUDSDialogue::RaiseNewSpeakerLine()
{
	for (const auto& Pair : Participants)
	{
		if (Pair.Value->GetClass()->ImplementsInterface(USUDSParticipant::StaticClass()))
		{
			ISUDSParticipant::Execute_OnDialogueSpeakerLine(Pair.Value, this);
		}
	}
	
	// Event listeners get it after
	OnSpeakerLine.Broadcast(this);
}

void USUDSDialogue::RaiseChoiceMade(int Index)
{
	for (const auto& Pair : Participants)
	{
		if (Pair.Value->GetClass()->ImplementsInterface(USUDSParticipant::StaticClass()))
		{
			ISUDSParticipant::Execute_OnDialogueChoiceMade(Pair.Value, this, Index);
		}
	}
	// Event listeners get it after
	OnChoice.Broadcast(this, Index);
}

PRAGMA_ENABLE_OPTIMIZATION
