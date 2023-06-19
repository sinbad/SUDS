// Copyright Steve Streeting 2022
// Released under the MIT license https://opensource.org/license/MIT/
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
