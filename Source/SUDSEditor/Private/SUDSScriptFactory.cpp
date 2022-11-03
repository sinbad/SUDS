#include "SUDSScriptFactory.h"

#include <string>

#include "SUDSScript.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "EditorFramework/AssetImportData.h"
#include "Internationalization/StringTable.h"
#include "Internationalization/StringTableCore.h"
#include "Internationalization/StringTableRegistry.h"

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
	if(Importer.ImportFromBuffer(Buffer, BufferEnd - Buffer, NameForErrors, false))
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
		Result->AssetImportData->Update(FactoryCurrentFilename);

	}

	GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, Result);

	return Result;
}
