// Copyright Steve Streeting 2022
// Released under the MIT license https://opensource.org/license/MIT/
#include "SUDSScriptFactory.h"

#include "PackageTools.h"
#include "SUDSEditorSettings.h"
#include "SUDSEditorVoiceOverTools.h"
#include "SUDSMessageLogger.h"
#include "SUDSScript.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "EditorFramework/AssetImportData.h"
#include "Internationalization/StringTable.h"
#include "Editor.h"
#include "Internationalization/StringTableCore.h"


USUDSScriptFactory::USUDSScriptFactory()
{
	SupportedClass = USUDSScript::StaticClass();
	bCreateNew = false;
	bEditorImport = true;
	bText = true;
	Formats.Add(TEXT("sud;SUDS Script File"));
}

UObject* USUDSScriptFactory::FactoryCreateText(UClass* InClass,
	UObject* InParent,
	FName InName,
	EObjectFlags Flags,
	UObject* Context,
	const TCHAR* Type,
	const TCHAR*& Buffer,
	const TCHAR* BufferEnd,
	FFeedbackContext* Warn)
{
	Flags |= RF_Transactional;
	FSUDSMessageLogger::ClearMessages();
	FSUDSMessageLogger Logger;

	USUDSScript* Result = nullptr;

	GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPreImport(this, InClass, InParent, InName, Type);
	
	const FString FactoryCurrentFilename = UFactory::GetCurrentFilename();
	FString CurrentSourcePath;
	FString FilenameNoExtension;
	FString UnusedExtension;
	FPaths::Split(FactoryCurrentFilename, CurrentSourcePath, FilenameNoExtension, UnusedExtension);
	const FString LongPackagePath = FPackageName::GetLongPackagePath(InParent->GetOutermost()->GetPathName());
	
	const FString NameForErrors(InName.ToString());

	// Now parse this using utility
	if(Importer.ImportFromBuffer(Buffer, BufferEnd - Buffer, NameForErrors, &Logger, false))
	{
		
		// Populate with data
		Result = NewObject<USUDSScript>(InParent, InName, Flags);
		UStringTable* StringTable = CreateStringTable(InName, Result, Flags, &Logger);
		Importer.PopulateAsset(Result, StringTable);
		
		// Register source info
		const FMD5Hash Hash = FSUDSScriptImporter::CalculateHash(Buffer, BufferEnd - Buffer);
		Result->AssetImportData->Update(FactoryCurrentFilename, Hash);

		// VO assets at import time?
		if (ShouldGenerateVoiceAssets(LongPackagePath))
		{
			FSUDSEditorVoiceOverTools::GenerateAssets(Result, Flags, &Logger);
		}

	}

	GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, Result);

	return Result;
}

bool USUDSScriptFactory::ShouldGenerateVoiceAssets(const FString& PackagePath) const
{
	if (auto Settings = GetDefault<USUDSEditorSettings>())
	{
		return Settings->ShouldGenerateVoiceAssets(PackagePath);
	}
	return false;
}

UStringTable* USUDSScriptFactory::CreateStringTable(FName InName, USUDSScript* Script, EObjectFlags Flags, FSUDSMessageLogger* Logger)
{
	auto Registry = &FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName).Get();

	const FString PackageDir = FPackageName::GetLongPackagePath(Script->GetOuter()->GetOutermost()->GetPathName());
	const FString StringTableName = InName.ToString() + "Strings";
	const FString PackageName = UPackageTools::SanitizePackageName(PackageDir / StringTableName);

	UStringTable* Table = nullptr;
	if (FPackageName::DoesPackageExist(*PackageName))
	{
		// Package already exists, so try and import over the top of it, if it doesn't already have a source file path
		TArray<FAssetData> Assets;
		if (Registry->GetAssetsByPackageName(*PackageName, Assets))
		{
			if (Assets.Num() > 0)
			{
				if (Assets[0].GetAsset()->IsA(UStringTable::StaticClass()))
				{
					Table = Cast<UStringTable>(Assets[0].GetAsset());
					Table->GetMutableStringTable()->ClearSourceStrings();
				}
				else
				{
					Logger->Logf(ELogVerbosity::Error,
								 TEXT(
									 "Asset %s already exists but is not a String Table! Cannot replace, please move aside and re-import script."),
								 *PackageName);
					return nullptr;
				}
			}
		}
	}

	if (!Table)
	{
		// String Table didn't exist (although package might have)
		// It's safe to call CreatePackage either way, it'll return the existing one if needed
		UPackage* Package = CreatePackage(*PackageName);
		if (!ensure(Package))
		{
			Logger->Logf(ELogVerbosity::Error,
						 TEXT("Failed to create/retrieve package for string table %s"),
						 *PackageName);
		}
		else
		{
			//Logger->Logf(ELogVerbosity::Display, TEXT("Creating string table %s"), *PackageName);

			// This constructor registers the string table with FStringTableRegistry
			Table = NewObject<UStringTable>(Package, FName(StringTableName), Flags);
			Package->FullyLoad();
			Table->MarkPackageDirty();
			FAssetRegistryModule::AssetCreated(Table);
		}
	}
		
	return Table;
	
}
