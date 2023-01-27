#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

struct FSUDSValue;
class USUDSDialogue;
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

protected:
	virtual void OnClose() override;

private:
	USUDSScript* Script = nullptr;
	TSharedPtr< STextBlock > DialogueText;
	USUDSDialogue* Dialogue = nullptr;

	void ExtendToolbar(FToolBarBuilder& ToolbarBuilder, TWeakPtr<SDockTab> Tab);
	void StartDialogue();

	void OnDialogueChoice(USUDSDialogue* Dialogue, int ChoiceIndex);
	void OnDialogueEvent(USUDSDialogue* Dialogue, FName EventName, const TArray<FSUDSValue>& Args);
	void OnDialogueFinished(USUDSDialogue* Dialogue);
	void OnDialogueProceeding(USUDSDialogue* Dialogue);
	void OnDialogueStarting(USUDSDialogue* Dialogue, FName LabelName);
	void OnDialogueSpeakerLine(USUDSDialogue* Dialogue);
	void OnDialogueVariableChanged(USUDSDialogue* Dialogue, FName VariableName, const FSUDSValue& ToValue, bool bFromScript);
	void OnDialogueVariableRequested(USUDSDialogue* Dialogue, FName VariableName);

};

#undef LOCTEXT_NAMESPACE