#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

class USUDSScript;

#define LOCTEXT_NAMESPACE "SUDS"

class FSUDSToolbarCommands
	: public TCommands<FSUDSToolbarCommands>
{
public:

	FSUDSToolbarCommands()
		: TCommands<FSUDSToolbarCommands>(
			"SUDS",
			LOCTEXT("SUDS", "Steves Unreal Dialogue System"),
			NAME_None, FEditorStyle::GetStyleSetName()
		)
	{ }

public:

	// TCommands interface

	virtual void RegisterCommands() override
	{
		UI_COMMAND(StartDialogue, "Start Dialogue", "Start/restart dialogue", EUserInterfaceActionType::Button, FInputChord());
	}

public:

	TSharedPtr<FUICommandInfo> StartDialogue;
};

class FSUDSEditorToolkit : public FAssetEditorToolkit
{
public:
	void InitEditor(const TArray<UObject*>& InObjects);

	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;

	virtual FText GetBaseToolkitName() const override;
	virtual FName GetToolkitFName() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	virtual FString GetWorldCentricTabPrefix() const override;

private:
	USUDSScript* Script = nullptr;

	void ExtendToolbar(FToolBarBuilder& ToolbarBuilder, TWeakPtr<SDockTab> Tab);
	void StartDialogue();

};

#undef LOCTEXT_NAMESPACE