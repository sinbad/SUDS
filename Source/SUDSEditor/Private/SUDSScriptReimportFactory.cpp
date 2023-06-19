// Copyright Steve Streeting 2022
// Released under the MIT license https://opensource.org/license/MIT/
#include "SUDSScriptReimportFactory.h"

#include "SUDSEditor.h"
#include "SUDSMessageLogger.h"
#include "SUDSScript.h"
#include "SUDSScriptNodeText.h"
#include "EditorFramework/AssetImportData.h"
#include "Sound/DialogueWave.h"

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

	// When a new script is created, it actually lives at the same address as the incoming one. UE must re-use objects
	// when you put them back at the same outer & asset name?
	// This means if we want to preserve anything from the previously imported object, such as generated VO asset links,
	// we need to copy those out now.
	TMap<FString, UDialogueVoice*> PrevSpeakerVoices = Script->GetSpeakerVoices();
	// Store the TextID -> DialogueWave, but also store the line text as well so we can detect whether it matches & warn if not
	TMap<FString, TPair<FString, UDialogueWave*> > PrevWaves;
	for (auto Node : Script->GetNodes())
	{
		if (auto TN = Cast<USUDSScriptNodeText>(Node))
		{
			if (auto W = TN->GetWave())
			{
				PrevWaves.Add(TN->GetTextID(), TPair<FString, UDialogueWave*>(TN->GetText().ToString(), W));
			}
		}
	}
	
	// Run the import again
	EReimportResult::Type Result = EReimportResult::Failed;
	bool OutCanceled = false;

	if (ImportObject(Script->GetClass(), Script->GetOuter(), *Script->GetName(), RF_Public | RF_Standalone, Filename, nullptr, OutCanceled) != nullptr)
	{
		UE_LOG(LogSUDSEditor, Log, TEXT("Imported successfully"));

		FSUDSMessageLogger Logger;
		// Now, try to restore the speaker voice / line wave links from before
		for (auto SpeakerID : Script->GetSpeakers())
		{
			if (auto pVoice = PrevSpeakerVoices.Find(SpeakerID))
			{
				Script->SetSpeakerVoice(SpeakerID, *pVoice);
			}
		}
		for (auto Node : Script->GetNodes())
		{
			if (auto TN = Cast<USUDSScriptNodeText>(Node))
			{
				if (auto pWavePair = PrevWaves.Find(TN->GetTextID()))
				{
					// Set the wave link either way
					TN->SetWave(pWavePair->Value);

					// Check that the text is the same; if it's not, then lines have potentially changed
					// we still live with the assignment we have (might be a minor edit) but user should be aware
					if (TN->GetText().ToString() != pWavePair->Key)
					{
						Logger.Logf(ELogVerbosity::Error,
						            TEXT(
							            "TextID %s is linked to Dialogue Wave %s, but text has changed. Check whether this line is linked to the correct wave, and consider Writing String Keys back to script before making more script changes in future."),
							            *TN->GetTextID(),
							            *pWavePair->Value->GetName());
					}

				}
			}
		}
		
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
