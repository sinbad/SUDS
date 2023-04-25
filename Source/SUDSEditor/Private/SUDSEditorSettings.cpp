#include "SUDSEditorSettings.h"

#include "SUDSScript.h"

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

FString USUDSEditorSettings::GetVoiceOutputDir(USUDSScript* Script) const
{
	const FString PackagePath = FPackageName::GetLongPackagePath(Script->GetOuter()->GetOutermost()->GetPathName());
	const FString ScriptName = Script->GetName();
	return GetVoiceOutputDir(PackagePath, ScriptName);
}

FString USUDSEditorSettings::GetVoiceOutputDir(const FString& PackagePath, const FString& ScriptName) const
{
	return GetOutputDir(DialogueVoiceAssetLocation, DialogueVoiceAssetSharedDir.Path, PackagePath, ScriptName);
}

FString USUDSEditorSettings::GetWaveOutputDir(USUDSScript* Script) const
{
	const FString PackagePath = FPackageName::GetLongPackagePath(Script->GetOuter()->GetOutermost()->GetPathName());
	const FString ScriptName = Script->GetName();
	return GetWaveOutputDir(PackagePath, ScriptName);
	
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
	// Deal with invalid SharedPath, final fallback
	if (Location == ESUDSAssetLocation::SharedDirectory &&
		!FPaths::DirectoryExists(SharedPath))
	{
		Location = ESUDSAssetLocation::ScriptDirectory;
	}
	else if (Location == ESUDSAssetLocation::SharedDirectorySubdir &&
		!FPaths::DirectoryExists(SharedPath))
	{
		Location = ESUDSAssetLocation::ScriptDirectorySubdir;
	}
	
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
