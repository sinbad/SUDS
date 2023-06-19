// Copyright Steve Streeting 2022
// Released under the MIT license https://opensource.org/license/MIT/
#include "SUDSEditor.h"

#include "ISettingsModule.h"
#include "ISettingsSection.h"
#include "SUDSEditorSettings.h"
#include "SUDSScriptActions.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"

#define LOCTEXT_NAMESPACE "FSUDSModule"

void FSUDSEditorModule::StartupModule()
{
	ScriptActions = MakeShared<FSUDSScriptActions>();
	FAssetToolsModule::GetModule().Get().RegisterAssetTypeActions(ScriptActions.ToSharedRef());

	auto SudsPlugin = IPluginManager::Get().FindPlugin(TEXT("SUDS"));
	if (SudsPlugin.IsValid())
	{
		const FString IconDir = SudsPlugin->GetBaseDir() + "/Content/Editor/Slate/Icons";
		const FVector2D Sz16 = FVector2D(16.0f, 16.0f);
		const FVector2D Sz64 = FVector2D(64.0f, 64.0f);

		StyleSet = MakeShared<FSlateStyleSet>("SUDSStyleSet");
		StyleSet->Set(TEXT("ClassIcon.SUDSScript"), new FSlateImageBrush(IconDir / TEXT("SUDSScript_16x.png"), Sz16));
		StyleSet->Set(TEXT("ClassThumbnail.SUDSScript"), new FSlateImageBrush(IconDir / TEXT("SUDSScript_64x.png"), Sz64));

		FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());
	}

	// register settings
	ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
	if (SettingsModule)
	{
		ISettingsSectionPtr SettingsSection = SettingsModule->RegisterSettings("Project", "Plugins", "SUDS Editor",
			LOCTEXT("SUDSEditorSettingsName", "SUDS Editor"),
			LOCTEXT("SUDSEditorSettingsDescription", "Configure the editor parts of SUDS."),
			GetMutableDefault<USUDSEditorSettings>()
		);
	}

	UE_LOG(LogSUDSEditor, Log, TEXT("SUDS Editor Module Started"))

}

void FSUDSEditorModule::ShutdownModule()
{
	if (StyleSet.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet.Get());
	}
	
	if (!FModuleManager::Get().IsModuleLoaded("AssetTools")) return;
		FAssetToolsModule::GetModule().Get().UnregisterAssetTypeActions(ScriptActions.ToSharedRef());
	
	UE_LOG(LogSUDSEditor, Log, TEXT("SUDS Editor Module Shut Down"))
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FSUDSEditorModule, SUDSEditor)
DEFINE_LOG_CATEGORY(LogSUDSEditor);
