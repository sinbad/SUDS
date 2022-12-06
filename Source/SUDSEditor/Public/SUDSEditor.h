// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"


class FSUDSScriptActions;
class FSlateStyleSet;

DECLARE_LOG_CATEGORY_EXTERN(LogSUDSEditor, Verbose, All);

class FSUDSEditorModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

protected:
	TSharedPtr<FSUDSScriptActions> ScriptActions;
	TSharedPtr<FSlateStyleSet> StyleSet;
};
