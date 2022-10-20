#include "SUDSEditor.h"
#include "SUDSScriptActions.h"

#define LOCTEXT_NAMESPACE "FSUDSModule"

void FSUDSEditorModule::StartupModule()
{
	ScriptActions = MakeShared<FSUDSScriptActions>();
	FAssetToolsModule::GetModule().Get().RegisterAssetTypeActions(ScriptActions.ToSharedRef());
}

void FSUDSEditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	if (!FModuleManager::Get().IsModuleLoaded("AssetTools")) return;
		FAssetToolsModule::GetModule().Get().UnregisterAssetTypeActions(ScriptActions.ToSharedRef());	
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FSUDSEditorModule, SUDSEditor)
DEFINE_LOG_CATEGORY(LogSUDSEditor);
