// Copyright Steve Streeting 2022
// Released under the MIT license https://opensource.org/license/MIT/
#pragma once

#include "CoreMinimal.h"
#include "Sound/DialogueVoice.h"
#include "UObject/Object.h"
#include "SUDSScript.generated.h"

class USUDSScriptNode;
class USUDSScriptNodeText;
class USUDSScriptNodeGosub;
/**
 * A single SUDS script asset.
 */
UCLASS(BlueprintType)
class SUDS_API USUDSScript : public UObject
{
	GENERATED_BODY()

protected:

	/// Array of nodes (static after import)
	UPROPERTY(BlueprintReadOnly, Category="SUDS")
	TArray<USUDSScriptNode*> Nodes;

	/// Map of labels to nodes
	UPROPERTY(BlueprintReadOnly, VisibleDefaultsOnly, Category="SUDS")
	TMap<FName, int> LabelList;

	// Header equivalents for startup
	UPROPERTY(BlueprintReadOnly, Category="SUDS")
	TArray<USUDSScriptNode*> HeaderNodes;

	UPROPERTY(BlueprintReadOnly, Category="SUDS")
	TMap<FName, int> HeaderLabelList;

	/// Array of all speaker IDs found in this script
	UPROPERTY(BlueprintReadOnly, VisibleDefaultsOnly, Category="SUDS")
	TArray<FString> Speakers;

	/// When using VO, Dialogue Voice assets are associated with speaker IDs
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category="SUDS")
	TMap<FString, UDialogueVoice*> SpeakerVoices;

	bool DoesAnyPathAfterLeadToChoice(USUDSScriptNode* FromNode);
	int RecurseLookForChoice(USUDSScriptNode* CurrNode);
	
public:
	void StartImport(TArray<USUDSScriptNode*>** Nodes,
	                 TArray<USUDSScriptNode*>** HeaderNodes,
	                 TMap<FName, int>** LabelList,
	                 TMap<FName, int>** ppHeaderLabelList,
	                 TArray<FString>** SpeakerList);
	void FinishImport();

	const TArray<USUDSScriptNode*>& GetNodes() const { return Nodes; }
	const TArray<USUDSScriptNode*>& GetHeaderNodes() const { return HeaderNodes; }
	const TMap<FName, int>& GetLabelList() const { return LabelList; }
	const TMap<FName, int>& GetHeaderLabelList() const { return HeaderLabelList; }
	

	/// Get the first header node, if any (header nodes are run every time the script starts)
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="SUDS")
	USUDSScriptNode* GetHeaderNode() const;

	/// Get the first node of the script, if starting from the beginning
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="SUDS")
	USUDSScriptNode* GetFirstNode() const;

	/// Get the next node after a given node, ONLY if there's only one way to go
	UFUNCTION(BlueprintCallable, Category="SUDS")
	USUDSScriptNode* GetNextNode(const USUDSScriptNode* Node) const;

	/// Get the first node of the script following a label, or null if the label wasn't found
	UFUNCTION(BlueprintCallable, Category="SUDS")
	USUDSScriptNode* GetNodeByLabel(const FName& Label) const;

	/// Try to find a speaker node by its text ID
	UFUNCTION(BlueprintCallable, Category="SUDS")
	USUDSScriptNodeText* GetNodeByTextID(const FString& TextID) const;
	/// Try to find a gosub node by its gosub ID
	UFUNCTION(BlueprintCallable, Category="SUDS")
	USUDSScriptNodeGosub* GetNodeByGosubID(const FString& ID) const;


	/// Get the list of speakers
	const TArray<FString>& GetSpeakers() const { return Speakers; }

	UFUNCTION(BlueprintCallable, Category="SUDS")
	UDialogueVoice* GetSpeakerVoice(const FString& SpeakerID) const;

	/// Set up the speaker voice association
	void SetSpeakerVoice(const FString& SpeakerID, UDialogueVoice* Voice);
	const TMap<FString, UDialogueVoice*> GetSpeakerVoices() const  { return SpeakerVoices; }

#if WITH_EDITORONLY_DATA
	// Import data for this 
	UPROPERTY(VisibleAnywhere, Instanced, Category=ImportSettings)
	TObjectPtr<class UAssetImportData> AssetImportData;
	
	// UObject interface
	virtual void PostInitProperties() override;
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
	virtual void GetAssetRegistryTags(FAssetRegistryTagsContext Context) const override;
#else
	virtual void GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const override;
#endif
	virtual void Serialize(FArchive& Ar) override;
	// End of UObject interface
#endif
	
};
