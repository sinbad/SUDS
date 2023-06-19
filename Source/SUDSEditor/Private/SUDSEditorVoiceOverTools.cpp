// Copyright Steve Streeting 2022
// Released under the MIT license https://opensource.org/license/MIT/
#include "SUDSEditorVoiceOverTools.h"

#include "ObjectTools.h"
#include "PackageTools.h"
#include "SUDSEditorSettings.h"
#include "SUDSMessageLogger.h"
#include "SUDSScript.h"
#include "SUDSScriptNodeText.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Internationalization/Regex.h"
#include "Sound/DialogueVoice.h"
#include "Sound/DialogueWave.h"
#include "Sound/SoundWave.h"


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
			Logger->Logf(ELogVerbosity::Error,
			             TEXT(
				             "Dialogue Voice assets are set to generate to a shared dir, but the dir '%s' is not valid."),
			             *Settings->DialogueVoiceAssetSharedDir.Path);
			bError = true;
		}
		if ((Settings->DialogueWaveAssetLocation == ESUDSAssetLocation::SharedDirectory || Settings->
				DialogueWaveAssetLocation == ESUDSAssetLocation::SharedDirectorySubdir) &&
			(Settings->DialogueWaveAssetSharedDir.Path.IsEmpty() ||
				!FPackageName::IsValidPath(Settings->DialogueWaveAssetSharedDir.Path)))
		{
			Logger->Logf(ELogVerbosity::Error,
			             TEXT(
				             "Dialogue Wave assets are set to generate to a shared dir, but the dir '%s' is not valid."),
			             *Settings->DialogueWaveAssetSharedDir.Path);
			bError = true;
		}
		if (bError) { return; }
	}
	else
	{
		Logger->Logf(ELogVerbosity::Error, TEXT("Unable to generate voice assets, settings not found"));
		return;
	}

	TMap<FString, UDialogueVoice*> UnsavedVoices;
	GenerateVoiceAssets(Script, Flags, Logger, UnsavedVoices);
	GenerateWaveAssets(Script, Flags, UnsavedVoices, Logger);
}

void FSUDSEditorVoiceOverTools::GenerateVoiceAssets(USUDSScript* Script, EObjectFlags Flags, FSUDSMessageLogger* Logger, TMap<FString, UDialogueVoice*> &OutCreatedVoices)
{
	auto Registry = &FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName).Get();

	FString Prefix;
	FString ParentDir;
	if (auto Settings = GetDefault<USUDSEditorSettings>())
	{
		Prefix = Settings->DialogueVoiceAssetPrefix;
		ParentDir = GetVoiceOutputDir(Script);
	}
	else
	{
		Logger->Logf(ELogVerbosity::Error, TEXT("Unable to generate voice assets, settings not found"));
		return;
	}

	for (auto Speaker : Script->GetSpeakers())
	{
		// All Dialogue Voice assets will be created in their own package
		FString PackageName;
		FString  AssetName;
		if (GetSpeakerVoiceAssetNames(Script, Speaker, PackageName, AssetName))
		{
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
							Logger->Logf(ELogVerbosity::Error,
										 TEXT(
											 "Asset %s already exists but is not a Dialogue Voice! Cannot replace, please move aside and re-import script."),
										 *PackageName);
						}
						else
						{
							// Make sure we still link the existing voice
							Script->SetSpeakerVoice(Speaker, Cast<UDialogueVoice>(Assets[0].GetAsset()));
							Script->MarkPackageDirty();
						}
						// Either way nothing more to do
						continue;
					}
				}
			}
		

			// If we got here then Dialogue Voice didn't exist (although package might have)
			// It's safe to call CreatePackage either way, it'll return the existing one if needed
			UPackage* Package = CreatePackage(*PackageName);
			if (!ensure(Package))
			{
				Logger->Logf(ELogVerbosity::Error,
							 TEXT("Failed to create/retrieve package for voice asset %s"),
							 *PackageName);
			}
			else
			{
				//Logger->Logf(ELogVerbosity::Display, TEXT("Creating voice asset %s"), *PackageName);

				UDialogueVoice* NewVoiceAsset = NewObject<UDialogueVoice>(Package, FName(AssetName), Flags);
				OutCreatedVoices.Add(Speaker, NewVoiceAsset);
				// there's nothing else to create here, voice is mostly a placeholder with the rest set up later by user
				FAssetRegistryModule::AssetCreated(NewVoiceAsset);

				Script->SetSpeakerVoice(Speaker, NewVoiceAsset);
				Script->MarkPackageDirty();

				Package->FullyLoad();
				NewVoiceAsset->MarkPackageDirty();
			}
		}
	}
}

bool FSUDSEditorVoiceOverTools::GetSpeakerVoicePackageName(USUDSScript* Script,
	const FString& SpeakerID,
	FString& OutPackageName)
{
	FString NotUsed;
	return GetSpeakerVoiceAssetNames(Script, SpeakerID, OutPackageName, NotUsed);
}

bool FSUDSEditorVoiceOverTools::GetSpeakerVoiceAssetNames(USUDSScript* Script,
	const FString& SpeakerID,
	FString& OutPackageName,
	FString& OutAssetName)
{
	if (auto Settings = GetDefault<USUDSEditorSettings>())
	{
		FString Prefix = Settings->DialogueVoiceAssetPrefix;
		FString ParentDir = GetVoiceOutputDir(Script);

		OutAssetName = FString::Printf(TEXT("%s%s"), *Prefix, *ObjectTools::SanitizeObjectName(SpeakerID));
		OutPackageName = UPackageTools::SanitizePackageName(ParentDir / OutAssetName);
		return true;
	}
	return false;
}

UDialogueVoice* FSUDSEditorVoiceOverTools::FindSpeakerVoice(USUDSScript* Script,
                                                            const FString& SpeakerID)
{
	FString PackageName;
	if (GetSpeakerVoicePackageName(Script, SpeakerID, PackageName))
	{
		
		auto Registry = &FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName).Get();

		if (FPackageName::DoesPackageExist(*PackageName))
		{
			// Package already exists, so try and import over the top of it, if it doesn't already have a source file path
			TArray<FAssetData> Assets;
			if (Registry->GetAssetsByPackageName(*PackageName, Assets))
			{
				if (Assets.Num() > 0)
				{
					if (Assets[0].GetAsset()->IsA(UDialogueVoice::StaticClass()))
					{
						return Cast<UDialogueVoice>(Assets[0].GetAsset());
					}
				}
			}
		}
	}
	return nullptr;
}

void FSUDSEditorVoiceOverTools::GenerateWaveAssets(USUDSScript* Script, EObjectFlags Flags, TMap<FString, UDialogueVoice*> UnsavedVoices, FSUDSMessageLogger* Logger)
{
	// We need to identify specific lines of dialogue to specify a wave asset to go with them, which means we
	// need to use the Text ID, just like we do for translations. This means that really, you shouldn't start to
	// assign sounds to wave assets until you've written the text IDs back to the script
	auto Registry = &FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName).Get();

	FString Prefix;
	FString ParentDir;
	if (auto Settings = GetDefault<USUDSEditorSettings>())
	{
		Prefix = FString::Printf(TEXT("%s%s_"), *Settings->DialogueWaveAssetPrefix, *GetScriptNameAsPrefix(Script));
		ParentDir = GetWaveOutputDir(Script);
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

			// Remove the '@' characters from the text ID for creating the asset name, they just turn into excessive '_'s
			const FRegexPattern TextIDPattern(TEXT("\\@([0-9a-fA-F]+)\\@"));
			FRegexMatcher TextIDRegex(TextIDPattern, TextID);
			if (TextIDRegex.FindNext())
			{
				TextID = TextIDRegex.GetCaptureGroup(1);
			}

			// All Dialogue Voice assets will be created in their own package
			const FString SanitizedName = FString::Printf(TEXT("%s%s"),
			                                              *Prefix,
			                                              *ObjectTools::SanitizeObjectName(TextID));
			const FString PackageName = UPackageTools::SanitizePackageName(ParentDir / SanitizedName);

			UDialogueWave* WaveAsset = nullptr;
			if (FPackageName::DoesPackageExist(*PackageName))
			{
				// Package already exists, so try and import over the top of it, if it doesn't already have a source file path
				TArray<FAssetData> Assets;
				if (Registry->GetAssetsByPackageName(*PackageName, Assets))
				{
					if (Assets.Num() > 0)
					{
						if (!Assets[0].GetAsset()->IsA(UDialogueWave::StaticClass()))
						{
							Logger->Logf(ELogVerbosity::Error,
							             TEXT(
								             "Asset %s already exists but is not a Dialogue Wave! Cannot replace, please move aside and re-import script."),
							             *PackageName);
							continue;
						}
						else
						{
							WaveAsset = Cast<UDialogueWave>(Assets[0].GetAsset());
						}
					}
				}
			}

			UDialogueVoice* SpeakerVoice = nullptr;
			// Finding speaker assets via FAssetRegistryModule does not work on unsaved assets, so we need to check
			// the unsaved voices that have been created in the previous step first
			if (auto pSV = UnsavedVoices.Find(Line->GetSpeakerID()))
			{
				SpeakerVoice = *pSV;
			}
			if (!SpeakerVoice)
			{
				// Now try assets
				SpeakerVoice = FindSpeakerVoice(Script, Line->GetSpeakerID());
			}
			if (!SpeakerVoice)
			{
				FString SpeakerPackageName;
				GetSpeakerVoicePackageName(Script, Line->GetSpeakerID(), SpeakerPackageName);
				Logger->Logf(ELogVerbosity::Error, TEXT("Error: speaker voice asset for %s is missing, expected to be at %s"), *Line->GetSpeakerID(), *SpeakerPackageName);
				continue;
			}

			UPackage* Package = CreatePackage(*PackageName);
			if (!ensure(Package))
			{
				Logger->Logf(ELogVerbosity::Error,
				             TEXT("Failed to create/retrieve package for wave asset %s"),
				             *PackageName);
			}
			else
			{
				FString NativeLangText = Line->GetText().ToString();
				USoundWave* SoundWave = nullptr;
				if (!WaveAsset)
				{
					//Logger->Logf(ELogVerbosity::Display, TEXT("Creating wave asset %s"), *PackageName);
					WaveAsset = NewObject<UDialogueWave>(Package, FName(SanitizedName), Flags);
					FAssetRegistryModule::AssetCreated(WaveAsset);
				}
				else
				{
					// We will always update the existing asset, but will warn if the text is changing
					if (WaveAsset->ContextMappings.Num() > 0)
					{
						auto Mapping = WaveAsset->ContextMappings[0];
						if (IsValid(Mapping.SoundWave))
						{
							SoundWave = Mapping.SoundWave;
							// This is OK if the context is what we were going to create anyway, we'll just leave it alone
							if (Mapping.Context.Speaker != SpeakerVoice ||
								WaveAsset->SpokenText != NativeLangText)
							{
								// No good, user has assigned sound to this dialogue wave, but we need to change it to
								// a different speaker / line
								Logger->Logf(ELogVerbosity::Error,
								             TEXT(
									             "Dialogue Wave %s has an assigned Sound Wave but the speaker or text has changed. You should update the sound wave!"),
								             *PackageName);
							}
						}
					}
				}


				// Set spoken text - native language only. Dialogue Wave handles the localisation of this separately,
				// we can't link it to our String Table unfortunately.
				WaveAsset->SpokenText = NativeLangText;
				// Set dialogue context
				// We need a Speaker Voice, we'll leave the sound and targets blank
				WaveAsset->UpdateContext(WaveAsset->ContextMappings[0], SoundWave, SpeakerVoice, TArray<UDialogueVoice*>());

				// Now assign the wave to the line
				Line->SetWave(WaveAsset);
				Script->MarkPackageDirty();

				Package->FullyLoad();
				WaveAsset->MarkPackageDirty();
			}
		}
	}
}

FString FSUDSEditorVoiceOverTools::GetVoiceOutputDir(USUDSScript* Script)
{
	if (auto Settings = GetDefault<USUDSEditorSettings>())
	{
		const FString PackagePath = FPackageName::GetLongPackagePath(Script->GetOuter()->GetOutermost()->GetPathName());
		const FString ScriptPrefix = GetScriptNameAsPrefix(Script);
		return Settings->GetVoiceOutputDir(PackagePath, ScriptPrefix);
	}
	return FString();
}

FString FSUDSEditorVoiceOverTools::GetWaveOutputDir(USUDSScript* Script)
{
	if (auto Settings = GetDefault<USUDSEditorSettings>())
	{
		const FString PackagePath = FPackageName::GetLongPackagePath(Script->GetOuter()->GetOutermost()->GetPathName());
		const FString ScriptPrefix = GetScriptNameAsPrefix(Script);
		return Settings->GetWaveOutputDir(PackagePath, ScriptPrefix);
	}
	return FString();
}



FString FSUDSEditorVoiceOverTools::GetScriptNameAsPrefix(USUDSScript* Script)
{
	FString Name = Script->GetName();

	if (auto Settings = GetDefault<USUDSEditorSettings>())
	{
		if (Settings->StripScriptPrefixesWhenGeneratingNames)
		{
			int32 Index = INDEX_NONE;
			if (Name.FindChar('_', Index))
			{
				if (Index < Name.Len() - 1)
				{
					Name = Name.RightChop(Index + 1);
				}
			}
		}
	}	
	return Name;
}
