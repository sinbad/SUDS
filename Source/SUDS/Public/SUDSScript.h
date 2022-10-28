#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SUDSScript.generated.h"

class USUDSScriptNode;
/**
 * A single SUDS script asset.
 */
UCLASS(BlueprintType)
class SUDS_API USUDSScript : public UObject
{
	GENERATED_BODY()

protected:

	/// Array of nodes (static after import)
	UPROPERTY(BlueprintReadOnly)
	TArray<USUDSScriptNode*> Nodes;

	/// Map of labels to nodes
	UPROPERTY(BlueprintReadOnly)
	TMap<FString, int> LabelList;
	
public:

	void StartImport(TArray<USUDSScriptNode*> **Nodes, TMap<FString, int> **LabelList);
	void FinishImport();

	/// Get the first node of the script, if starting from the beginning
	UFUNCTION(BlueprintCallable, BlueprintPure)
	USUDSScriptNode* GetFirstNode() const;

	/// Get the first node of the script following a label, or null if the label wasn't found
	UFUNCTION(BlueprintCallable, BlueprintPure)
	USUDSScriptNode* GetNodeByLabel(const FString& Label) const;


#if WITH_EDITORONLY_DATA
	// Import data for this 
	UPROPERTY(VisibleAnywhere, Instanced, Category=ImportSettings)
	TObjectPtr<class UAssetImportData> AssetImportData;
	
	// UObject interface
	virtual void PostInitProperties() override;
	virtual void GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const override;
	virtual void Serialize(FArchive& Ar) override;
	// End of UObject interface
#endif
	
};
