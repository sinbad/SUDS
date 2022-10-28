#include "SUDSScript.h"

#include "EditorFramework/AssetImportData.h"

void USUDSScript::StartImport(TArray<USUDSScriptNode*>** ppNodes, TMap<FString, int>** ppLabelList)
{
	*ppNodes = &Nodes;
	*ppLabelList = &LabelList;
}

void USUDSScript::FinishImport()
{
}

USUDSScriptNode* USUDSScript::GetFirstNode() const
{
	if (Nodes.Num() > 0)
		return Nodes[0];

	return nullptr;
}

USUDSScriptNode* USUDSScript::GetNodeByLabel(const FString& Label) const
{
	if (const int* pIdx = LabelList.Find(Label))
	{
		return Nodes[*pIdx];
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
