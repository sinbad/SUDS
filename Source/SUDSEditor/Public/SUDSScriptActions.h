// Copyright Steve Streeting 2022
// Released under the MIT license https://opensource.org/license/MIT/
#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"
#include "SUDSScriptNodeGosub.h"
#include "SUDSScriptNodeText.h"

class USUDSScriptNode;
class USUDSScript;
struct FSUDSMessageLogger;
/**
 * 
 */
class FSUDSScriptActions : public FAssetTypeActions_Base
{
public:
	virtual FText GetName() const override;
	virtual FString GetObjectDisplayName(UObject* Object) const override;
	virtual UClass* GetSupportedClass() const override;
	virtual FColor GetTypeColor() const override;
	virtual uint32 GetCategories() override;
	virtual void GetResolvedSourceFilePaths(const TArray<UObject*>& TypeAssets,
		TArray<FString>& OutSourceFilePaths) const override;
	virtual bool IsImportedAsset() const override { return true; }
	virtual void GetActions(const TArray<UObject*>& InObjects, FToolMenuSection& Section) override;
	virtual bool HasActions(const TArray<UObject*>& InObjects) const override;
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects,
		TSharedPtr<IToolkitHost> EditWithinLevelEditor) override;

protected:
	void WriteBackTextIDs(TArray<TWeakObjectPtr<USUDSScript>> Scripts);

	void GenerateVOAssets(TArray<TWeakObjectPtr<USUDSScript>> Scripts);
		
};
