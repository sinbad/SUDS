#include "SUDSEditor.h"
#include "SUDSScriptActions.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"

#define LOCTEXT_NAMESPACE "FSUDSModule"

void FSUDSEditorModule::StartupModule()
{
	ScriptActions = MakeShared<FSUDSScriptActions>();
	FAssetToolsModule::GetModule().Get().RegisterAssetTypeActions(ScriptActions.ToSharedRef());

	const FString IconDir = IPluginManager::Get().FindPlugin(TEXT("SUDS"))->GetBaseDir() + "/Content/Editor/Slate/Icons";
	const FVector2D Sz16 = FVector2D(16.0f, 16.0f);
	const FVector2D Sz64 = FVector2D(64.0f, 64.0f);

	StyleSet = MakeShared<FSlateStyleSet>("SUDSStyleSet");
	StyleSet->Set(TEXT("ClassIcon.SUDSScript"), new FSlateImageBrush(IconDir / TEXT("SUDSScript_16x.png"), Sz16));
	StyleSet->Set(TEXT("ClassThumbnail.SUDSScript"), new FSlateImageBrush(IconDir / TEXT("SUDSScript_64x.png"), Sz64));

	FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());
}

void FSUDSEditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet.Get());
	
	if (!FModuleManager::Get().IsModuleLoaded("AssetTools")) return;
		FAssetToolsModule::GetModule().Get().UnregisterAssetTypeActions(ScriptActions.ToSharedRef());	
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FSUDSEditorModule, SUDSEditor)
DEFINE_LOG_CATEGORY(LogSUDSEditor);
