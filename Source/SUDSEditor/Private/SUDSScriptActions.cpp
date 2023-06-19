// Copyright Steve Streeting 2022
// Released under the MIT license https://opensource.org/license/MIT/
#include "SUDSScriptActions.h"

#include "SUDSEditorScriptTools.h"
#include "SUDSEditorSettings.h"
#include "SUDSEditorToolkit.h"
#include "SUDSEditorVoiceOverTools.h"
#include "SUDSMessageLogger.h"
#include "SUDSScript.h"
#include "ToolMenuSection.h"
#include "EditorFramework/AssetImportData.h"

FText FSUDSScriptActions::GetName() const
{
	return INVTEXT("SUDS Script");
}

FString FSUDSScriptActions::GetObjectDisplayName(UObject* Object) const
{
	return Object->GetName();
}

UClass* FSUDSScriptActions::GetSupportedClass() const
{
	return USUDSScript::StaticClass();
}

FColor FSUDSScriptActions::GetTypeColor() const
{
	return FColor::Orange;
}

uint32 FSUDSScriptActions::GetCategories()
{
	return EAssetTypeCategories::Misc;
}

void FSUDSScriptActions::GetResolvedSourceFilePaths(const TArray<UObject*>& TypeAssets,
	TArray<FString>& OutSourceFilePaths) const
{
	for (auto& Asset : TypeAssets)
	{
		const auto Script = CastChecked<USUDSScript>(Asset);
		if (Script->AssetImportData)
		{
			Script->AssetImportData->ExtractFilenames(OutSourceFilePaths);
		}
	}
}

bool FSUDSScriptActions::HasActions(const TArray<UObject*>& InObjects) const
{
	return true;
}

void FSUDSScriptActions::OpenAssetEditor(const TArray<UObject*>& InObjects,
	TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	MakeShared<FSUDSEditorToolkit>()->InitEditor(InObjects);
}

void FSUDSScriptActions::GetActions(const TArray<UObject*>& InObjects, FToolMenuSection& Section)
{
	const auto Scripts = GetTypedWeakObjectPtrs<USUDSScript>(InObjects);

	Section.AddMenuEntry(
		"WriteBackTextIDs",
		NSLOCTEXT("SUDS", "WriteBackTextIDs", "Write Back String Keys"),
		NSLOCTEXT("SUDS",
		          "WriteBackTextIDsTooltip",
		          "Write string table keys back to source script to make them constant for localisation."),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Details"),
		FUIAction(
			FExecuteAction::CreateSP(this, &FSUDSScriptActions::WriteBackTextIDs, Scripts),
			FCanExecuteAction()
		)
	);
	Section.AddMenuEntry(
		"GenerateVOAssets",
		NSLOCTEXT("SUDS", "GenerateVOAssets", "Generate Voice Assets"),
		NSLOCTEXT("SUDS",
				  "GenerateVOAssetsTooltip",
				  "Generate Dialogue Voice / Dialogue Wave assets for the selected scripts"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Toolbar.Export"),
		FUIAction(
			FExecuteAction::CreateSP(this, &FSUDSScriptActions::GenerateVOAssets, Scripts),
			FCanExecuteAction()
		)
	);

}

void FSUDSScriptActions::WriteBackTextIDs(TArray<TWeakObjectPtr<USUDSScript>> Scripts)
{
	if (FMessageDialog::Open(EAppMsgType::YesNo,
						 FText::FromString(
							 "Are you sure you want to write string keys back to the selected scripts?"))
	== EAppReturnType::Yes)
	{
		FSUDSMessageLogger Logger;
		for (auto Script : Scripts)
		{
			if (Script.IsValid())
			{
				FSUDSEditorScriptTools::WriteBackTextIDs(Script.Get(), Logger);
			}
		}
	}
}


void FSUDSScriptActions::GenerateVOAssets(TArray<TWeakObjectPtr<USUDSScript>> Scripts)
{
	if (FMessageDialog::Open(EAppMsgType::YesNo,
	                         FText::FromString(
		                         "Are you sure you want to generate Dialogue Voice / Dialogue Wave assets for the selected scripts?"))
		== EAppReturnType::Yes)
	{
		EObjectFlags Flags = RF_Public | RF_Standalone | RF_Transactional;
		FSUDSMessageLogger Logger;
		for (auto WeakScript : Scripts)
		{
			if (auto Script = WeakScript.Get())
			{
				FSUDSEditorVoiceOverTools::GenerateAssets(Script, Flags, &Logger);
			}
		}
	}
}
