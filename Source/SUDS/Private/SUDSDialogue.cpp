#include "SUDSDialogue.h"

#include "SUDSScript.h"
#include "SUDSScriptNode.h"

DEFINE_LOG_CATEGORY(LogSUDSDialogue)

const FText USUDSDialogue::DummyText = FText::FromString("INVALID");

USUDSDialogue::USUDSDialogue()
{
}

void USUDSDialogue::Initialise(const USUDSScript* Script, FName StartLabel)
{
	BaseScript = Script;
	if (StartLabel != NAME_None)
	{
		SetCurrentNode(BaseScript->GetNodeByLabel(StartLabel));
	}
	else
	{
		SetCurrentNode(BaseScript->GetFirstNode());
	}
}

void USUDSDialogue::SetCurrentNode(USUDSScriptNode* Node)
{
	CurrentNode = Node;

	CurrentSpeakerDisplayName = FText::GetEmpty();
	CurrentText = FText::GetEmpty();
	AllCurrentChoices = nullptr;
	ValidCurrentChoices.Reset();

}

FText USUDSDialogue::GetCurrentText() const
{
	if (CurrentText.IsEmpty())
	{
		// TODO: localisation
	}
	// For now, just use temp text
	return CurrentNode->GetTempText();
}

const FString& USUDSDialogue::GetCurrentSpeakerID() const
{
	return CurrentSpeakerID;
}

FText USUDSDialogue::GetCurrentSpeakerDisplayName() const
{
	if (CurrentSpeakerDisplayName.IsEmpty())
	{
		// TODO: derive speaker display name
	}
	return CurrentSpeakerDisplayName;
}

const TArray<FSUDSScriptEdge>* USUDSDialogue::GetChoices(bool bOnlyValidChoices) const
{
	// Cache choices
	if (!AllCurrentChoices)
	{
		// Choice nodes are combinatorial, but still separate so you can link to them multiple text nodes (e.g. loops)
		if (CurrentNode && CurrentNode->GetEdgeCount() == 1)
		{
			auto Edge = CurrentNode->GetEdge(0);
			if (Edge->Navigation == ESUDSScriptEdgeNavigation::Combine && Edge->TargetNode.IsValid())
			{
				check(Edge->TargetNode->GetNodeType() == ESUDSScriptNodeType::Choice);
				/// Choices are the edges under the choice node
				AllCurrentChoices = &Edge->TargetNode->GetEdges();

				if (bOnlyValidChoices)
				{
					// Note that we only re-evaluate conditions once per node change, for stability between counts / indexes
					// Even if conditions are dependent on external data we only sample them once when node changes so everything is consistent
					// TODO: evaluate conditions
					ValidCurrentChoices.Reset();
					for (auto& Choice : *AllCurrentChoices)
					{
						// Copy happens here
						ValidCurrentChoices.Add(Choice);
					}
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
			// TODO localisation string table
			return FText::FromString((*Choices)[Index].TempText);
		}
		else
		{
			UE_LOG(LogSUDSDialogue, Error, TEXT("Invalid choice index %d on node %s"), Index, *GetCurrentText().ToString());
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
			if ((*Choices)[Index].TargetNode.IsValid())
			{
				SetCurrentNode((*Choices)[Index].TargetNode.Get());
				return !IsEnded();
			}
			else
			{
				// Reached the end
				SetCurrentNode(nullptr);
				return false;
			}
		}
		else
		{
			UE_LOG(LogSUDSDialogue, Error, TEXT("Invalid choice index %d on node %s"), Index, *GetCurrentText().ToString());
		}
	}
	return false;
}

bool USUDSDialogue::IsEnded() const
{
	return CurrentNode == nullptr;
}
