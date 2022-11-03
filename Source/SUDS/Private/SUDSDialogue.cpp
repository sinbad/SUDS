#include "SUDSDialogue.h"

#include "SUDSScript.h"
#include "SUDSScriptNode.h"

DEFINE_LOG_CATEGORY(LogSUDSDialogue)

PRAGMA_DISABLE_OPTIMIZATION

const FText USUDSDialogue::DummyText = FText::FromString("INVALID");
const FString USUDSDialogue::DummyString = "INVALID";

USUDSDialogue::USUDSDialogue()
{
}

void USUDSDialogue::Initialise(const USUDSScript* Script, FName StartLabel)
{
	BaseScript = Script;
	Restart(true, StartLabel);
}

void USUDSDialogue::SetCurrentNode(USUDSScriptNode* Node)
{
	CurrentNode = Node;

	CurrentSpeakerDisplayName = FText::GetEmpty();
	AllCurrentChoices = nullptr;
	ValidCurrentChoices.Reset();

}

FText USUDSDialogue::GetCurrentText() const
{
	// For now, just use temp text
	return CurrentNode->GetText();
}

const FString& USUDSDialogue::GetCurrentSpeakerID() const
{
	if (CurrentNode)
		return CurrentNode->GetSpeakerID();
	
	return DummyString;
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
			return (*Choices)[Index].Text;
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

void USUDSDialogue::Restart(bool bResetState, FName StartLabel)
{
	if (bResetState)
	{
		// TODO: reset state
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
		SetCurrentNode(StartNode);
	}
	else
	{
		SetCurrentNode(BaseScript->GetFirstNode());
	}
	
}

PRAGMA_ENABLE_OPTIMIZATION
