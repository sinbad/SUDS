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
	TMap<FName, int> LabelList;

	// Header equivalents for startup
	TArray<USUDSScriptNode*> HeaderNodes;
	TMap<FName, int> HeaderLabelList;

	/// Array of all speaker IDs found in this script
	UPROPERTY(BlueprintReadOnly)
	TArray<FString> Speakers;
	
public:
	void StartImport(TArray<USUDSScriptNode*>** Nodes,
	                 TArray<USUDSScriptNode*>** HeaderNodes,
	                 TMap<FName, int>** LabelList,
	                 TMap<FName, int>** ppHeaderLabelList,
	                 TArray<FString>** SpeakerList);
	void FinishImport();

	/// Get the first header node, if any (header nodes are run every time the script starts)
	UFUNCTION(BlueprintCallable, BlueprintPure)
	USUDSScriptNode* GetHeaderNode() const;

	/// Get the first node of the script, if starting from the beginning
	UFUNCTION(BlueprintCallable, BlueprintPure)
	USUDSScriptNode* GetFirstNode() const;

	/// Get the next node after a given node, ONLY if there's only one way to go
	UFUNCTION(BlueprintCallable)
	USUDSScriptNode* GetNextNode(USUDSScriptNode* Node) const;

	/// Get the first node of the script following a label, or null if the label wasn't found
	UFUNCTION(BlueprintCallable, BlueprintPure)
	USUDSScriptNode* GetNodeByLabel(const FName& Label) const;

	/// Get the list of speakers
	const TArray<FString>& GetSpeakers() const { return Speakers; }

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
