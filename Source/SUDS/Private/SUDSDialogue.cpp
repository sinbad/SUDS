#include "SUDSDialogue.h"

#include "SUDSParticipant.h"
#include "SUDSScript.h"
#include "SUDSScriptNode.h"

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
	CurrentNode = nullptr;
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

void USUDSDialogue::SetCurrentNode(USUDSScriptNode* Node)
{
	CurrentNode = Node;

	CurrentSpeakerDisplayName = FText::GetEmpty();
	AllCurrentChoices = nullptr;
	ValidCurrentChoices.Reset();
	bParamNamesExtracted = false;

	RaiseNewSpeakerLine();

}

FText USUDSDialogue::GetText() const
{
	if (CurrentNode->HasParameters())
	{
		return FText::Format(CurrentNode->GetTextFormat(), VariableState);
	}
	else
	{
		return CurrentNode->GetText();
	}
}

const FString& USUDSDialogue::GetSpeakerID() const
{
	if (CurrentNode)
		return CurrentNode->GetSpeakerID();
	
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

const TArray<FSUDSScriptEdge>* USUDSDialogue::GetChoices(bool bOnlyValidChoices) const
{
	// "CurrentNode" is always a text node in practice
	// other nodes are passed through but not alighted on
	// Cache choices
	if (!AllCurrentChoices)
	{
		ValidCurrentChoices.Reset();
		if (CurrentNode && CurrentNode->GetEdgeCount() == 1)
		{
			auto Edge = CurrentNode->GetEdge(0);
			// Choice nodes are combinatorial, but still separate so you can link to them multiple text nodes (e.g. loops)
			if (Edge->GetNavigation() == ESUDSScriptEdgeNavigation::Combine && Edge->GetTargetNode().IsValid())
			{
				check(Edge->GetTargetNode()->GetNodeType() == ESUDSScriptNodeType::Choice);
				/// Choices are the edges under the choice node
				AllCurrentChoices = &Edge->GetTargetNode()->GetEdges();

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
			else
			{
				// Simple no-choice progression (text->text)
				ValidCurrentChoices.Add(*Edge);			
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
		if (CurrentNode && CurrentNode->GetEdgeCount() == 1)
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
			
			if ((*Choices)[Index].GetTargetNode().IsValid())
			{
				SetCurrentNode((*Choices)[Index].GetTargetNode().Get());
				return !IsEnded();
			}
			else
			{
				// Reached the end
				SetCurrentNode(nullptr);
				OnFinished.Broadcast(this);
				return false;
			}
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
	return CurrentNode == nullptr;
}

void USUDSDialogue::Restart(bool bResetState, FName StartLabel)
{
	if (bResetState)
	{
		VariableState.Reset();
	}

	RaiseStarting(StartLabel);

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
		SetCurrentNode(StartNode);
	}
	else
	{
		SetCurrentNode(BaseScript->GetFirstNode());
	}
	
}


TSet<FString> USUDSDialogue::GetParametersInUse()
{
	// Build on demand, may not be needed
	if (!bParamNamesExtracted)
	{
		CurrentRequestedParamNames.Reset();
		if (CurrentNode && CurrentNode->HasParameters())
		{
			CurrentRequestedParamNames.Append(CurrentNode->GetParameterNames());
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
