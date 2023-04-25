#include "SUDSScriptReimportFactory.h"

#include "SUDSEditor.h"
#include "SUDSScript.h"
#include "EditorFramework/AssetImportData.h"

USUDSScriptReimportFactory::USUDSScriptReimportFactory()
{
	SupportedClass = USUDSScript::StaticClass();
	bCreateNew = false;
	// We need to have a unique priority vs the original factory, so go after
	ImportPriority = DefaultImportPriority - 1;
}

bool USUDSScriptReimportFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
	USUDSScript* Script = Cast<USUDSScript>(Obj);
	if (Script && Script->AssetImportData)
	{
		Script->AssetImportData->ExtractFilenames(OutFilenames);
		return true;
	}
	return false;
}

void USUDSScriptReimportFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{
	USUDSScript* Script = Cast<USUDSScript>(Obj);
	if (Script && ensure(NewReimportPaths.Num() == 1))
	{
		Script->AssetImportData->UpdateFilenameOnly(NewReimportPaths[0]);
	}
	
}

EReimportResult::Type USUDSScriptReimportFactory::Reimport(UObject* Obj)
{
	USUDSScript* Script = Cast<USUDSScript>(Obj);
	if (!Script)
	{
		return EReimportResult::Failed;
	}

	// Make sure file is valid and exists
	const FString Filename = Script->AssetImportData->GetFirstFilename();
	if (!Filename.Len() || IFileManager::Get().FileSize(*Filename) == INDEX_NONE)
	{
		return EReimportResult::Failed;
	}

	// Run the import again
	EReimportResult::Type Result = EReimportResult::Failed;
	bool OutCanceled = false;

	if (ImportObject(Script->GetClass(), Script->GetOuter(), *Script->GetName(), RF_Public | RF_Standalone, Filename, nullptr, OutCanceled) != nullptr)
	{
		UE_LOG(LogSUDSEditor, Log, TEXT("Imported successfully"));

		Script->AssetImportData->Update(Filename);
		
		// Try to find the outer package so we can dirty it up
		if (Script->GetOuter())
		{
			Script->GetOuter()->MarkPackageDirty();
		}
		else
		{
			Script->MarkPackageDirty();
		}
		Result = EReimportResult::Succeeded;
	}
	else
	{
		if (OutCanceled)
		{
			UE_LOG(LogSUDSEditor, Warning, TEXT("-- import canceled"));
		}
		else
		{
			UE_LOG(LogSUDSEditor, Warning, TEXT("-- import failed"));
		}

		Result = EReimportResult::Failed;
	}

	return Result;
}

int32 USUDSScriptReimportFactory::GetPriority() const
{
	return ImportPriority;
}
