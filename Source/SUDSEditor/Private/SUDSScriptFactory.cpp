#include "SUDSScriptFactory.h"

#include "FileHelpers.h"
#include "SUDSEditorSettings.h"
#include "SUDSMessageLogger.h"
#include "SUDSScript.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "EditorFramework/AssetImportData.h"
#include "Internationalization/StringTable.h"

PRAGMA_DISABLE_OPTIMIZATION

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

		// VO assets
		FString VoiceDir;
		FString WaveDir;
		if (ShouldGenerateVoiceAssets(LongPackagePath, FilenameNoExtension, VoiceDir, WaveDir))
		{
			Importer.GenerateVoices(Result, VoiceDir, Flags, &Logger);
		}

	}

	GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, Result);

	return Result;
}

bool USUDSScriptFactory::ShouldGenerateVoiceAssets(const FString& PackagePath, const FString& ScriptName, FString& OutVoiceDir, FString& OutWaveDir) const
{
	if (auto Settings = GetDefault<USUDSEditorSettings>())
	{
		if (Settings->ShouldGenerateVoiceAssets(PackagePath))
		{
			OutVoiceDir = Settings->GetVoiceOutputDir(PackagePath, ScriptName);
			OutWaveDir = Settings->GetWaveOutputDir(PackagePath, ScriptName);
			return true;
		}
	}
	return false;
}

PRAGMA_ENABLE_OPTIMIZATION
