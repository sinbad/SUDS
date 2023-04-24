#include "SUDSScriptFactory.h"

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

	}

	GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, Result);

	return Result;
}
PRAGMA_ENABLE_OPTIMIZATION