#pragma once

#include "CoreMinimal.h"
#include "SUDSValue.h"
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


class FSUDSEditorOutputRow
{
public:
	FText Prefix;
	FText Line;
	FSlateColor PrefixColour;
	FSlateColor LineColour;

	FSUDSEditorOutputRow(const FText& InPrefix,
	                     const FText& InLine,
	                     const FSlateColor& InPrefixColour = FSlateColor::UseForeground(),
	                     const FSlateColor& InLineColour = FSlateColor::UseForeground()) :
		Prefix(InPrefix),
		Line(InLine),
		PrefixColour(InPrefixColour),
		LineColour(InLineColour)
	{
	}

	
};

class FSUDSEditorVariableRow
{
public:
	FName Name;
	FSUDSValue Value;
	FSUDSEditorVariableRow(const FName& InName, const FSUDSValue& InValue) : Name(InName), Value(InValue) {}
};

class SSUDSEditorVariableItem : public SMultiColumnTableRow< TSharedPtr<FString> >
{
public:	
	SLATE_BEGIN_ARGS(SSUDSEditorVariableItem)
	{}
	SLATE_ARGUMENT(float, InitialWidth)
	SLATE_ARGUMENT(FName, VariableName)
	SLATE_ARGUMENT(FSUDSValue, VariableValue)
SLATE_END_ARGS()

public:

	/**
	 * Construct this widget.  Called by the SNew() Slate macro.
	 *
	 * @param	InArgs	          Declaration used by the SNew() macro to construct this widget.
	 * @oaram   InOwnerTableView  The owner table into which this row is being placed.
	 */
	void Construct( const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView );

	/**
	 * Generates the widget for the specified column.
	 *
	 * @param ColumnName The name of the column to generate the widget for.
	 * @return The widget.
	 */
	virtual TSharedRef<SWidget> GenerateWidgetForColumn( const FName& ColumnName ) override;
protected:
	float InitialWidth = 70;
	FName VariableName;
	FSUDSValue VariableValue;
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
	USUDSDialogue* Dialogue = nullptr;
	float VarColumnWidth = 75;
	// FSUDSEditorDialogueRow needs to held by a TSharedPtr for SListView
	TSharedPtr<SListView<TSharedPtr<FSUDSEditorOutputRow>>> OutputListView;
	TArray<TSharedPtr<FSUDSEditorOutputRow>> OutputRows;
	TSharedPtr<SVerticalBox> ChoicesBox;
	TSharedPtr<SListView<TSharedPtr<FSUDSEditorVariableRow>>> VariablesListView;
	TArray<TSharedPtr<FSUDSEditorVariableRow>> VariableRows;

	const FSlateColor SpeakerColour = FLinearColor(1.0f, 1.0f, 0.6f, 1.0f);
	const FSlateColor ChoiceColour = FLinearColor(0.4f, 1.0f, 0.4f, 1.0f);
	const FSlateColor EventColour = FLinearColor(0.2f, 0.6f, 1.0f, 1.0f);
	const FSlateColor StartColour = FLinearColor(1.0f, 0.5f, 0.0f, 1.0f);
	const FSlateColor FinishColour = FLinearColor(1.0f, 0.3f, 0.3f, 1.0f);

	void ExtendToolbar(FToolBarBuilder& ToolbarBuilder, TWeakPtr<SDockTab> Tab);
	void UpdateVariables();
	void StartDialogue();
	void UpdateOutput();
	void UpdateChoiceButtons();

	void OnDialogueChoice(USUDSDialogue* Dialogue, int ChoiceIndex);
	void OnDialogueEvent(USUDSDialogue* Dialogue, FName EventName, const TArray<FSUDSValue>& Args);
	void OnDialogueFinished(USUDSDialogue* Dialogue);
	void OnDialogueProceeding(USUDSDialogue* Dialogue);
	void OnDialogueStarting(USUDSDialogue* Dialogue, FName LabelName);
	void OnDialogueSpeakerLine(USUDSDialogue* Dialogue);
	void OnDialogueVariableChanged(USUDSDialogue* Dialogue, FName VariableName, const FSUDSValue& ToValue, bool bFromScript);
	void OnDialogueVariableRequested(USUDSDialogue* Dialogue, FName VariableName);
	
	TSharedRef<ITableRow> OnGenerateRowForDialogue(
		TSharedPtr<FSUDSEditorOutputRow> FsudsEditorDialogueRow,
		const TSharedRef<STableViewBase>& TableViewBase);
	TSharedRef<ITableRow> OnGenerateRowForVariable(TSharedPtr<FSUDSEditorVariableRow> Row,
	                                               const TSharedRef<STableViewBase>& Table);

};

#undef LOCTEXT_NAMESPACE