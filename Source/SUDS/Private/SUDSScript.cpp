// Copyright Steve Streeting 2022
// Released under the MIT license https://opensource.org/license/MIT/
#include "SUDSScript.h"

#include "SUDSScriptNode.h"
#include "SUDSScriptNodeGosub.h"
#include "SUDSScriptNodeText.h"
#include "EditorFramework/AssetImportData.h"

void USUDSScript::StartImport(TArray<USUDSScriptNode*>** ppNodes,
                              TArray<USUDSScriptNode*>** ppHeaderNodes,
                              TMap<FName, int>** ppLabelList,
                              TMap<FName, int>** ppHeaderLabelList,
                              TArray<FString>** ppSpeakerList)
{
	*ppNodes = &Nodes;
	*ppHeaderNodes = &HeaderNodes;
	*ppLabelList = &LabelList;
	*ppHeaderLabelList = &HeaderLabelList;
	*ppSpeakerList = &Speakers;
}

USUDSScriptNode* USUDSScript::GetNextNode(const USUDSScriptNode* Node) const
{
	switch (Node->GetEdgeCount())
	{
	case 0:
		return nullptr;
	case 1:
		return Node->GetEdge(0)->GetTargetNode().Get();
	default:
		UE_LOG(LogSUDS, Error, TEXT("Called GetNextNode on a node with more than one edge"));
		return nullptr;
	}
	
}


#define kChoiceFound 1
#define kChoiceNotFoundBeforeText -1
#define kChoiceNotFoundBeforeEnd 0 


bool USUDSScript::DoesAnyPathAfterLeadToChoice(USUDSScriptNode* FromNode)
{
	// Look for any possible choice following a node (text or gosub)
	// If it's possible to find a choice in one of the paths ahead, before another text node, the return true
	// Given that there might be conditionals, not all paths might lead to a choice, but we only care if one of them does
	// We recurse into conditional paths where they exist until we know.
	// For a gosub this is looking for the next after a return, not inside the sub
	USUDSScriptNode* CurrNode = GetNextNode(FromNode);

	return RecurseLookForChoice(CurrNode) == kChoiceFound;
}


int USUDSScript::RecurseLookForChoice(USUDSScriptNode* CurrNode)
{
	// Return int so that we can differentiate:
	// 1  = we found a choice
	// 0  = we didn't find a choice, but also didn't hit another text node (reached end, or gosub return)
	// -1 = we hit a text node
	while (CurrNode)
	{
		switch (CurrNode->GetNodeType())
		{
		case ESUDSScriptNodeType::Text:
			// if we hit a text node, there was no choice
			return kChoiceNotFoundBeforeText;
		case ESUDSScriptNodeType::Choice:
			// we found a choice
			return kChoiceFound;
		case ESUDSScriptNodeType::Select:
			{
				// Explore all possible routes
				int WorstResult = kChoiceNotFoundBeforeEnd;
				for (auto& Edge : CurrNode->GetEdges())
				{
					auto TargetNode = Edge.GetTargetNode();
					if (TargetNode.IsValid())
					{
						const int ConditionalPath = RecurseLookForChoice(TargetNode.Get());
						if (ConditionalPath == kChoiceFound)
							return kChoiceFound;
						WorstResult = FMath::Min(ConditionalPath, WorstResult);
					}
				}
				return WorstResult;
			}
		case ESUDSScriptNodeType::Event:
		case ESUDSScriptNodeType::SetVariable:
			CurrNode = GetNextNode(CurrNode); 
			break;
		case ESUDSScriptNodeType::Gosub:
			// When we hit a gosub here we go into it, not after it
			if (auto GosubNode = Cast<USUDSScriptNodeGosub>(CurrNode))
			{
				int SubResult = RecurseLookForChoice(GetNodeByLabel(GosubNode->GetLabelName()));
				if (SubResult != 0)
				{
					// Found definitive result (choice or text) inside sub
					return SubResult;
				}
			}
			// Otherwise, we didn't conclude within the sub, continue following it
			CurrNode = GetNextNode(CurrNode);
			break;
		default: ;
		case ESUDSScriptNodeType::Return:
			// this is when we're exploring a sub for the choice
			return kChoiceNotFoundBeforeEnd; 
		};
	}

	return kChoiceNotFoundBeforeEnd;
}

void USUDSScript::FinishImport()
{
	// As an optimisation, make all text/gosub nodes pre-scan their follow-on nodes for choice nodes
	// We can actually have intermediate nodes, for example set nodes which run for all choices that are placed
	// between the text and the first choice. Resolve whether they exist now
	for (auto Node : Nodes)
	{
		if (Node->GetNodeType() == ESUDSScriptNodeType::Text ||
			Node->GetNodeType() == ESUDSScriptNodeType::Gosub)
		{
			if (DoesAnyPathAfterLeadToChoice(Node))
			{
				switch (Node->GetNodeType())
				{
				case ESUDSScriptNodeType::Text:
					{
						if (auto TextNode = Cast<USUDSScriptNodeText>(Node))
						{
							TextNode->NotifyMayHaveChoices();
						}
						break;
					}
				case ESUDSScriptNodeType::Gosub:
					{
						if (auto GosubNode = Cast<USUDSScriptNodeGosub>(Node))
						{
							GosubNode->NotifyMayHaveChoices();
						}
						break;
					}
				default: break;
				}
			}
		}
	}
	
}

USUDSScriptNode* USUDSScript::GetHeaderNode() const
{
	if (HeaderNodes.Num() > 0)
		return HeaderNodes[0];

	return nullptr;
}

USUDSScriptNode* USUDSScript::GetFirstNode() const
{
	if (Nodes.Num() > 0)
		return Nodes[0];

	return nullptr;
}

USUDSScriptNode* USUDSScript::GetNodeByLabel(const FName& Label) const
{
	if (const int* pIdx = LabelList.Find(Label))
	{
		return Nodes[*pIdx];
	}

	return nullptr;
	
}

USUDSScriptNodeText* USUDSScript::GetNodeByTextID(const FString& TextID) const
{
	for (auto N : Nodes)
	{
		if (N->GetNodeType() == ESUDSScriptNodeType::Text)
		{
			if (auto TN = Cast<USUDSScriptNodeText>(N))
			{
				if (TextID.Equals(TN->GetTextID()))
				{
					return TN;
				}
			}
		}
	}
	return nullptr;
}

USUDSScriptNodeGosub* USUDSScript::GetNodeByGosubID(const FString& ID) const
{
	for (auto N : Nodes)
	{
		if (N->GetNodeType() == ESUDSScriptNodeType::Text)
		{
			if (auto GN = Cast<USUDSScriptNodeGosub>(N))
			{
				if (ID.Equals(GN->GetGosubID()))
				{
					return GN;
				}
			}
		}
	}
	return nullptr;
}

UDialogueVoice* USUDSScript::GetSpeakerVoice(const FString& SpeakerID) const
{
	if (UDialogueVoice* const* pVoice = SpeakerVoices.Find(SpeakerID))
	{
		return *pVoice;
	}
	return nullptr;
}

void USUDSScript::SetSpeakerVoice(const FString& SpeakerID, UDialogueVoice* Voice)
{
	SpeakerVoices.Add(SpeakerID, Voice);
}

#if WITH_EDITORONLY_DATA

void USUDSScript::PostInitProperties()
{
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		AssetImportData = NewObject<UAssetImportData>(this, TEXT("AssetImportData"));
	}
	Super::PostInitProperties();
}

void USUDSScript::GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const
{
	if (AssetImportData)
	{
		OutTags.Add( FAssetRegistryTag(SourceFileTagName(), AssetImportData->GetSourceData().ToJson(), FAssetRegistryTag::TT_Hidden) );
	}

	Super::GetAssetRegistryTags(OutTags);
}
void USUDSScript::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if (Ar.IsLoading() && Ar.UEVer() < VER_UE4_ASSET_IMPORT_DATA_AS_JSON && !AssetImportData)
	{
		// AssetImportData should always be valid
		AssetImportData = NewObject<UAssetImportData>(this, TEXT("AssetImportData"));
	}
}
#endif
