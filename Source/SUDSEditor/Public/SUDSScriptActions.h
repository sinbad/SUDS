#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"
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
protected:
	void WriteBackTextIDs(TArray<TWeakObjectPtr<USUDSScript>> Scripts);
	void WriteBackTextIDs(USUDSScript* Script, FSUDSMessageLogger& Logger);
	bool WriteBackTextIDsFromNodes(const TArray<USUDSScriptNode*> Nodes, TArray<FString>& Lines, const FString& NameForErrors, FSUDSMessageLogger& Logger);
	bool WriteBackTextID(const FText& AssetText, int LineNo, TArray<FString>& Lines, const FString& NameForErrors, FSUDSMessageLogger& Logger);
	bool TextIDCheckMatch(const FText& AssetText, const FString& SourceLine);
		
};
