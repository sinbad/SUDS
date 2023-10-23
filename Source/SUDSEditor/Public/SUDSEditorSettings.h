// Copyright Steve Streeting 2022
// Released under the MIT license https://opensource.org/license/MIT/
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SUDSEditorSettings.generated.h"


UENUM(BlueprintType)
enum class ESUDSAssetLocation : uint8
{
	/// Use a single flat shared directory
	SharedDirectory,
	/// Use a shared base directory, but create subfolders based on the script asset name
	SharedDirectorySubdir,
	/// Place asset alongside the script that originated it
	ScriptDirectory,
	/// Place asset in a subfolder of the folder containing the script that generated it, named the same as the script
	ScriptDirectorySubdir
};

class USUDSScript;
/**
 * Settings for editor-specific aspects of SUDS (no effect at runtime)
 */
UCLASS(config = Editor, defaultconfig, meta=(DisplayName="SUDS Editor"))
class SUDSEDITOR_API USUDSEditorSettings : public UObject
{
	GENERATED_BODY()
public:

	UPROPERTY(config, EditAnywhere, Category = "Voice", meta = (Tooltip = "Where to place Dialogue Voice assets for speakers in scripts when generated", RelativeToGameContentDir, LongPackageName))
	ESUDSAssetLocation DialogueVoiceAssetLocation = ESUDSAssetLocation::SharedDirectory;
	
	UPROPERTY(config, EditAnywhere, Category = "Voice", meta = (Tooltip = "Shared directory for Dialogue Voice assets, if using a shared directory", RelativeToGameContentDir, LongPackageName))
	FDirectoryPath DialogueVoiceAssetSharedDir;

	UPROPERTY(config, EditAnywhere, Category = "Voice", meta = (Tooltip = "Where to place Dialogue Wave assets for speaker lines in scripts", RelativeToGameContentDir, LongPackageName))
	ESUDSAssetLocation DialogueWaveAssetLocation = ESUDSAssetLocation::ScriptDirectorySubdir;
	
	UPROPERTY(config, EditAnywhere, Category = "Voice", meta = (Tooltip = "Shared directory for Dialogue Wave assets, if using a shared directory", RelativeToGameContentDir, LongPackageName))
	FDirectoryPath DialogueWaveAssetSharedDir;
	
	UPROPERTY(config, EditAnywhere, Category = "Voice", meta = (Tooltip = "Prefix to give Dialogue Voice assets in front of their SpeakerID", RelativeToGameContentDir, LongPackageName))
	FString DialogueVoiceAssetPrefix = "DV_";
	UPROPERTY(config, EditAnywhere, Category = "Voice", meta = (Tooltip = "Prefix to give Dialogue Wave assets in front of their SpeakerID", RelativeToGameContentDir, LongPackageName))
	FString DialogueWaveAssetPrefix = "DW_";
	UPROPERTY(config, EditAnywhere, Category = "Voice", meta = (Tooltip = "When generating subdirectories and wave asset names from script names, whether to strip characters before the first '_' to avoid including script prefix", RelativeToGameContentDir, LongPackageName))
	bool StripScriptPrefixesWhenGeneratingNames = true;

	UPROPERTY(config, EditAnywhere, Category = "Voice", AdvancedDisplay, meta = (Tooltip = "Whether to auto-generate Dialogue Voice/Wave assets for ALL dialogue scripts on import. Note: you can always generate VO assets manually."))
	bool AlwaysAutoGenerateVoiceOverAssetsOnImport = false;

	UPROPERTY(config, EditAnywhere, Category = "Voice", AdvancedDisplay, meta = (Tooltip = "Auto-generate Dialogue Voice/Wave assets for scripts in these directories (and subdirectories) on import. Note: you can always generate VO assets manually.", RelativeToGameContentDir, LongPackageName))
	TArray<FDirectoryPath> DirectoriesToAutoGenerateVoiceOverAssetsOnImport;
	
	UPROPERTY(config, EditAnywhere, Category = "Choices", meta = (Tooltip = "Whether to generate a spoken line for choices (default false)."))
	bool AlwaysGenerateSpeakerLinesFromChoices = false;

	UPROPERTY(config, EditAnywhere, Category = "Choices", meta = (Tooltip = "The SpeakerID to use for speaker lines generated from choices"))
	FString SpeakerIdForGeneratedLinesFromChoices = "Player";

	USUDSEditorSettings() {}

	bool ShouldGenerateVoiceAssets(const FString& PackagePath) const;
	FString GetVoiceOutputDir(const FString& PackagePath, const FString& ScriptName) const;
	FString GetWaveOutputDir(const FString& PackagePath, const FString& ScriptName) const;
	static FString GetOutputDir(ESUDSAssetLocation Location, const FString& SharedPath, const FString& PackagePath, const FString& ScriptName);
};
