#include "SUDSEditorVoiceOverTools.h"

#include "ObjectTools.h"
#include "PackageTools.h"
#include "SUDSEditorSettings.h"
#include "SUDSMessageLogger.h"
#include "SUDSScript.h"
#include "SUDSScriptNodeText.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Sound/DialogueVoice.h"

PRAGMA_DISABLE_OPTIMIZATION

void FSUDSEditorVoiceOverTools::GenerateAssets(USUDSScript* Script, EObjectFlags Flags, FSUDSMessageLogger* Logger)
{

	// First check for problems
	if (auto Settings = GetDefault<USUDSEditorSettings>())
	{
		bool bError = false;
		if ((Settings->DialogueVoiceAssetLocation == ESUDSAssetLocation::SharedDirectory || Settings->
				DialogueVoiceAssetLocation == ESUDSAssetLocation::SharedDirectorySubdir) &&
			(Settings->DialogueVoiceAssetSharedDir.Path.IsEmpty() ||
				!FPackageName::IsValidPath(Settings->DialogueVoiceAssetSharedDir.Path)))
		{
			Logger->Logf(ELogVerbosity::Error, TEXT("Dialogue Voice assets are set to generate to a shared dir, but the dir '%s' is not valid."), *Settings->DialogueVoiceAssetSharedDir.Path);
			bError = true;
		}
		if ((Settings->DialogueWaveAssetLocation == ESUDSAssetLocation::SharedDirectory || Settings->
				DialogueWaveAssetLocation == ESUDSAssetLocation::SharedDirectorySubdir) &&
			(Settings->DialogueWaveAssetSharedDir.Path.IsEmpty() ||
				!FPackageName::IsValidPath(Settings->DialogueWaveAssetSharedDir.Path)))
		{
			Logger->Logf(ELogVerbosity::Error, TEXT("Dialogue Wave assets are set to generate to a shared dir, but the dir '%s' is not valid."), *Settings->DialogueWaveAssetSharedDir.Path);
			bError = true;
		}
		if (bError)
		{
			return;
		}
	}
	else
	{
		Logger->Logf(ELogVerbosity::Error, TEXT("Unable to generate voice assets, settings not found"));
		return;
	}

	GenerateVoiceAssets(Script, Flags, Logger);
	GenerateWaveAssets(Script, Flags, Logger);
}

void FSUDSEditorVoiceOverTools::GenerateVoiceAssets(USUDSScript* Script, EObjectFlags Flags, FSUDSMessageLogger* Logger)
{
	auto Registry = &FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName).Get();

	FString Prefix;
	FString ParentDir;
	if (auto Settings = GetDefault<USUDSEditorSettings>())
	{
		Prefix = Settings->DialogueVoiceAssetPrefix;
		ParentDir = Settings->GetVoiceOutputDir(Script);
	}
	else
	{
		Logger->Logf(ELogVerbosity::Error, TEXT("Unable to generate voice assets, settings not found"));
		return;
	}
	
	for (auto Speaker : Script->GetSpeakers())
	{
		// All Dialogue Voice assets will be created in their own package
		const FString SanitizedName = FString::Printf(TEXT("%s%s"), *Prefix, *ObjectTools::SanitizeObjectName(Speaker));
		const FString PackageName = UPackageTools::SanitizePackageName(ParentDir / SanitizedName);

		if (FPackageName::DoesPackageExist(*PackageName))
		{
			// Package already exists, so try and import over the top of it, if it doesn't already have a source file path
			TArray<FAssetData> Assets;
			if (Registry->GetAssetsByPackageName(*PackageName, Assets))
			{
				if (Assets.Num() > 0)
				{
					if (!Assets[0].GetAsset()->IsA(UDialogueVoice::StaticClass()))
					{
						Logger->Logf(ELogVerbosity::Error, TEXT("Asset %s already exists but is not a Dialogue Voice! Cannot replace, please move aside and re-import script."), *PackageName);
					}
					// Either way nothing to do
					continue;
				}
			}
		}

		// If we got here then Dialogue Voice didn't exist (although package might have)
		// It's safe to call CreatePackage either way, it'll return the existing one if needed
		UPackage* Package = CreatePackage(*PackageName);
		if (!ensure(Package))
		{
			Logger->Logf(ELogVerbosity::Error, TEXT("Failed to create/retrieve package for voice asset %s"), *PackageName);
		}
		else
		{
			//Logger->Logf(ELogVerbosity::Display, TEXT("Creating voice asset %s"), *PackageName);

			UDialogueVoice* NewVoiceAsset = NewObject<UDialogueVoice>(Package, FName(SanitizedName), Flags);
			// there's nothing else to create here, voice is mostly a placeholder with the rest set up later by user
			FAssetRegistryModule::AssetCreated(NewVoiceAsset);

			Script->SetSpeakerVoice(Speaker, NewVoiceAsset);
			Script->MarkPackageDirty();

			Package->FullyLoad();
			NewVoiceAsset->MarkPackageDirty();
		}
		
	}
	
}

void FSUDSEditorVoiceOverTools::GenerateWaveAssets(USUDSScript* Script, EObjectFlags Flags, FSUDSMessageLogger* Logger)
{
	// We need to identify specific lines of dialogue to specify a wave asset to go with them, which means we
	// need to use the Text ID, just like we do for translations. This means that really, you shouldn't start to
	// assign sounds to wave assets until you've written the text IDs back to the script, which is also when you're
	// update the text on the wave assets if different, and
	// if there's been a sound asset linked, throw a warning that you'll have to update the sound link. We should document
	// that you should write back IDs to the script before linking any sound assets, since the IDs can change otherwise,
	// just like for translation.

	auto Registry = &FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName).Get();

	FString Prefix;
	FString ParentDir;
	if (auto Settings = GetDefault<USUDSEditorSettings>())
	{
		Prefix = FString::Printf(TEXT("%s%s_"), *Settings->DialogueWaveAssetPrefix, *Script->GetName());
		ParentDir = Settings->GetWaveOutputDir(Script);
	}
	else
	{
		Logger->Logf(ELogVerbosity::Error, TEXT("Unable to generate wave assets, settings not found"));
		return;
	}

	for (auto Node : Script->GetNodes())
	{
		if (auto Line = Cast<USUDSScriptNodeText>(Node))
		{
			// We use the TextID to identify a specific line, just like for translation
			FString TextID = Line->GetTextID();
		}
	}


	
}

PRAGMA_ENABLE_OPTIMIZATION
