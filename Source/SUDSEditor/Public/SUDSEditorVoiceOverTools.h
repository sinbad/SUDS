// Copyright Steve Streeting 2022
// Released under the MIT license https://opensource.org/license/MIT/
#pragma once

#include "CoreMinimal.h"
#include "SUDSScriptNodeText.h"
#include "Sound/DialogueVoice.h"


struct FSUDSMessageLogger;
class USUDSScript;

class SUDSEDITOR_API FSUDSEditorVoiceOverTools
{
public:
	static void GenerateAssets(USUDSScript* Script, EObjectFlags Flags, FSUDSMessageLogger* Logger);
protected:
	static void GenerateVoiceAssets(USUDSScript* Script,
	                                EObjectFlags Flags,
	                                FSUDSMessageLogger *Logger,
	                                TMap<FString, UDialogueVoice*> &OutCreatedVoices);
	static void GenerateWaveAssets(USUDSScript* Script, EObjectFlags Flags, TMap<FString, UDialogueVoice*>, FSUDSMessageLogger* Logger);
	static bool GetSpeakerVoicePackageName(USUDSScript* Script, const FString& SpeakerID, FString& OutPackageName);
	static bool GetSpeakerVoiceAssetNames(USUDSScript* Script,
	                               const FString& SpeakerID,
	                               FString& OutPackageName,
	                               FString& OutAssetName);
	static UDialogueVoice* FindSpeakerVoice(USUDSScript* Script, const FString& SpeakerID);
	static FString GetScriptNameAsPrefix(USUDSScript* Script);

	static FString GetVoiceOutputDir(USUDSScript* Script);
	static FString GetWaveOutputDir(USUDSScript* Script);

};
