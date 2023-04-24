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
/**
 * Settings for editor-specific aspects of SUDS (no effect at runtime)
 */
UCLASS(config = Editor, defaultconfig, meta=(DisplayName="SUDS Editor"))
class SUDSEDITOR_API USUDSEditorSettings : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(config, EditAnywhere, Category = SUDS, meta = (Tooltip = "Whether to generate VO assets for ALL dialogue scripts (ignores all other VO generation options"))
	bool AlwaysGenerateVOAssets = false;

	UPROPERTY(config, EditAnywhere, Category = SUDS, meta = (Tooltip = "Generate VO assets for scripts in these directories (and subdirectories)", RelativeToGameContentDir, LongPackageName))
	TArray<FDirectoryPath> DirectoriesToGenerateVOAssets;

	UPROPERTY(config, EditAnywhere, Category = SUDS, meta = (Tooltip = "Where to place Dialogue Voice assets for speakers in scripts", RelativeToGameContentDir, LongPackageName))
	ESUDSAssetLocation DialogueVoiceAssetLocation = ESUDSAssetLocation::SharedDirectory;
	
	UPROPERTY(config, EditAnywhere, Category = SUDS, meta = (Tooltip = "Shared directory for Dialogue Voice assets, if using a shared directory", RelativeToGameContentDir, LongPackageName))
	FDirectoryPath DialogueVoiceAssetSharedDir;

	UPROPERTY(config, EditAnywhere, Category = SUDS, meta = (Tooltip = "Where to place Dialogue Wave assets for speaker lines in scripts", RelativeToGameContentDir, LongPackageName))
	ESUDSAssetLocation DialogueWaveAssetLocation = ESUDSAssetLocation::ScriptDirectorySubdir;
	
	UPROPERTY(config, EditAnywhere, Category = SUDS, meta = (Tooltip = "Shared directory for Dialogue Wave assets, if using a shared directory", RelativeToGameContentDir, LongPackageName))
	FDirectoryPath DialogueWaveAssetSharedDir;
	
	USUDSEditorSettings() {}
	
};
