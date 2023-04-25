#pragma once

#include "CoreMinimal.h"


struct FSUDSMessageLogger;
class USUDSScript;

class SUDSEDITOR_API FSUDSEditorVoiceOverTools
{
public:
	static void GenerateAssets(USUDSScript* Script, EObjectFlags Flags, FSUDSMessageLogger* Logger);
protected:
	static void GenerateVoiceAssets(USUDSScript* Script,
	                                EObjectFlags Flags,
	                                FSUDSMessageLogger *Logger);
	static void GenerateWaveAssets(USUDSScript* Script, EObjectFlags Flags, FSUDSMessageLogger* Logger);
};
