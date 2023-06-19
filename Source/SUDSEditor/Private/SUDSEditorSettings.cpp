// Copyright Steve Streeting 2022
// Released under the MIT license https://opensource.org/license/MIT/
#include "SUDSEditorSettings.h"

bool USUDSEditorSettings::ShouldGenerateVoiceAssets(const FString& PackagePath) const
{
	if (AlwaysAutoGenerateVoiceOverAssetsOnImport)
		return true;

	for (auto Dir : DirectoriesToAutoGenerateVoiceOverAssetsOnImport)
	{
		if (FPaths::IsUnderDirectory(PackagePath, Dir.Path))
		{
			return true;
		}
	}
	return false;
}

FString USUDSEditorSettings::GetVoiceOutputDir(const FString& PackagePath, const FString& ScriptName) const
{
	return GetOutputDir(DialogueVoiceAssetLocation, DialogueVoiceAssetSharedDir.Path, PackagePath, ScriptName);
}

FString USUDSEditorSettings::GetWaveOutputDir(const FString& PackagePath, const FString& ScriptName) const
{
	return GetOutputDir(DialogueWaveAssetLocation, DialogueWaveAssetSharedDir.Path, PackagePath, ScriptName);
}

FString USUDSEditorSettings::GetOutputDir(ESUDSAssetLocation Location,
                                          const FString& SharedPath,
                                          const FString& PackagePath,
                                          const FString& ScriptName)
{
	switch(Location)
	{
	default:
	case ESUDSAssetLocation::SharedDirectory:
		return SharedPath;
	case ESUDSAssetLocation::SharedDirectorySubdir:
		return FPaths::Combine(SharedPath, ScriptName);
	case ESUDSAssetLocation::ScriptDirectory:
		return PackagePath;
	case ESUDSAssetLocation::ScriptDirectorySubdir:
		return FPaths::Combine(PackagePath, ScriptName);
	}
}
