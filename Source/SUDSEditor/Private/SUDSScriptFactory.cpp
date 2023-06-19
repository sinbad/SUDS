// Copyright Steve Streeting 2022
// Released under the MIT license https://opensource.org/license/MIT/
#include "SUDSScriptFactory.h"

#include "SUDSEditorSettings.h"
#include "SUDSEditorVoiceOverTools.h"
#include "SUDSMessageLogger.h"
#include "SUDSScript.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "EditorFramework/AssetImportData.h"
#include "Internationalization/StringTable.h"


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
		// Build native language string table
		const FName StringTableName = FName(InName.ToString() + "Strings");
		// This constructor registers the string table with FStringTableRegistry
		UStringTable* StringTable = NewObject<UStringTable>(InParent, StringTableName, Flags);
		Importer.PopulateAsset(Result, StringTable);
		
		FAssetRegistryModule::AssetCreated(StringTable);

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
