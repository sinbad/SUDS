#include "SUDSScript.h"

#include "SUDSScriptNode.h"
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

const USUDSScriptNode* USUDSScript::GetNextChoiceNode(const USUDSScriptNode* FromTextNode) const
{
	// there is *always* an initial choice node under a text node if there are choices
	// however, it might not be the *only* choice node; if there are conditionals it could be a mixed tree
	// of choice / select nodes. But there will always be a root choice
	// However, there might be set/event nodes between them, if that's where they were scripted (execute for any choices)
	if (FromTextNode && FromTextNode->GetNodeType() == ESUDSScriptNodeType::Text)
	{
		auto NextNode = GetNextNode(FromTextNode);
		// We skip over nodes which can be executed in between text & choice (set, event)
		while (NextNode &&
			(NextNode->GetNodeType() == ESUDSScriptNodeType::SetVariable ||
			NextNode->GetNodeType() == ESUDSScriptNodeType::Event))
		{
			NextNode = GetNextNode(NextNode);
		}

		if (NextNode && NextNode->GetNodeType() == ESUDSScriptNodeType::Choice)
		{
			return NextNode;
		}

	}

	return nullptr;
	
}


void USUDSScript::FinishImport()
{
	// As an optimisation, make all text nodes pre-scan their follow-on nodes for choice nodes
	// We can actually have intermediate nodes, for example set nodes which run for all choices that are placed
	// between the text and the first choice. Resolve whether they exist now
	for (auto Node : Nodes)
	{
		if (auto ChoiceNode = GetNextChoiceNode(Node))
		{
			auto TextNode = Cast<USUDSScriptNodeText>(Node);
			TextNode->NotifyHasChoices();
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
				if (TextID.Equals(FTextInspector::GetTextId(TN->GetText()).GetKey().GetChars()))
				{
					return TN;
				}
			}
		}
	}
	return nullptr;
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
