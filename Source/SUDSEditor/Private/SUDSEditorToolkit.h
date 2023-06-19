// Copyright Steve Streeting 2022
// Released under the MIT license https://opensource.org/license/MIT/
#pragma once

#include "CoreMinimal.h"
#include "SUDSValue.h"
#include "Framework/Text/BaseTextLayoutMarshaller.h"
#include "UObject/Object.h"

class SMultiLineEditableTextBox;
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
			NAME_None,
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION > 0
			FAppStyle::GetAppStyleSetName()
#else
			FEditorStyle::GetStyleSetName()
#endif
		)
	{ }

public:

	// TCommands interface

	virtual void RegisterCommands() override
	{
		UI_COMMAND(StartDialogue, "Start Dialogue", "Start/restart dialogue", EUserInterfaceActionType::Button, FInputChord());
		UI_COMMAND(WriteBackTextIDs, "Write String Keys", "Write string keys back to script source to stabilise for localisation / voice asset links", EUserInterfaceActionType::Button, FInputChord());
		UI_COMMAND(GenerateVOAssets, "Generate Voice Assets", "Generate DialogueVoice and DialogueWave assets for VO", EUserInterfaceActionType::Button, FInputChord());
	}

public:

	TSharedPtr<FUICommandInfo> StartDialogue;
	TSharedPtr<FUICommandInfo> WriteBackTextIDs;
	TSharedPtr<FUICommandInfo> GenerateVOAssets;
};


class FSUDSEditorOutputRow
{
public:
	FText Prefix;
	FText Line;
	FSlateColor PrefixColour;
	FSlateColor LineColour;
	FSlateColor BgColour;

	FSUDSEditorOutputRow(const FText& InPrefix,
	                     const FText& InLine,
	                     const FSlateColor& InPrefixColour,
	                     const FSlateColor& InLineColour,
	                     const FSlateColor& InBgColour) :
		Prefix(InPrefix),
		Line(InLine),
		PrefixColour(InPrefixColour),
		LineColour(InLineColour),
		BgColour(InBgColour)
	{
	}

	
};

class SSUDSEditorOutputItem : public SMultiColumnTableRow< TSharedPtr<FString> >
{
public:	
	SLATE_BEGIN_ARGS(SSUDSEditorOutputItem)
	{}
	SLATE_ARGUMENT(float, InitialWidth)
	SLATE_ARGUMENT(FText, Prefix)
	SLATE_ARGUMENT(FText, Line)
	SLATE_ARGUMENT(FSlateColor, PrefixColour)
	SLATE_ARGUMENT(FSlateColor, LineColour)
	SLATE_ARGUMENT(FSlateColor, BgColour)
	
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
	FText Prefix;
	FText Line;
	FSlateColor PrefixColour;
	FSlateColor LineColour;
};


class FSUDSEditorVariableRow
{
public:
	FName Name;
	FSUDSValue Value;
	bool bIsManualOverride;

	FSUDSEditorVariableRow(const FName& InName, const FSUDSValue& InValue, bool bIsManual) : Name(InName),
		Value(InValue),
		bIsManualOverride(bIsManual)
	{
	}

	friend bool operator<(const FSUDSEditorVariableRow& Lhs, const FSUDSEditorVariableRow& RHS)
	{
		return Lhs.Name.LexicalLess(RHS.Name);
	}

	friend bool operator<(const TSharedPtr<FSUDSEditorVariableRow>& Lhs, const TSharedPtr<FSUDSEditorVariableRow>& RHS)
	{
		return *Lhs < *RHS;
	}

	

};

class SSUDSEditorVariableItem : public SMultiColumnTableRow< TSharedPtr<FString> >
{
public:	
	SLATE_BEGIN_ARGS(SSUDSEditorVariableItem)
	{}
	SLATE_ARGUMENT(float, InitialWidth)
	SLATE_ARGUMENT(FName, VariableName)
	SLATE_ARGUMENT(FSUDSValue, VariableValue)
	SLATE_ARGUMENT(bool, bIsManualOverride)
	SLATE_ARGUMENT(class FSUDSEditorToolkit*, Parent)
SLATE_END_ARGS()

public:

	/**
	 * Construct this widget.  Called by the SNew() Slate macro.
	 *
	 * @param	InArgs	          Declaration used by the SNew() macro to construct this widget.
	 * @oaram   InOwnerTableView  The owner table into which this row is being placed.
	 */
	void Construct( const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView );

public:
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
	bool bIsManualOverride;
	class FSUDSEditorToolkit* Parent = nullptr;

	TSharedRef<class SWidget>  GetGenderMenu();
	void OnGenderSelected(ETextGender TextGender);
	virtual FVector2D ComputeDesiredSize(float) const override;

};

struct FSUDSTraceLogMessage
{
	TSharedRef<FString> Message;
	FName Category;
	FSlateColor Colour;

	FSUDSTraceLogMessage(FName InCategory, const FString& InMessage, const FSlateColor& InColour)
		: Message(MakeShared<FString>(InMessage))
		, Category(InCategory)
		, Colour(InColour)
	{
	}
};


class FSUDSTraceLogMarshaller : public FBaseTextLayoutMarshaller
{
public:

	FSUDSTraceLogMarshaller();

	// ITextLayoutMarshaller
	virtual void SetText(const FString& SourceString, FTextLayout& TargetTextLayout) override;
	virtual void GetText(FString& TargetString, const FTextLayout& SourceTextLayout) override;

	void AppendMessage(FName InCategory, int LineNo, const FString& Message, const FSlateColor& Colour);
	void ClearMessages();
protected:
	TArray< TSharedPtr<FSUDSTraceLogMessage> > Messages;	
};

// Trace log widget, really just so we can tick and automatically scroll to the end
class SSUDSTraceLog	: public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSUDSTraceLog)
	{}
	SLATE_END_ARGS()

	SSUDSTraceLog() : bIsUserScrolled(false) {}

	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	void AppendMessage(FName InCategory, int LineNo, const FString& Message, const FSlateColor& Colour);
	void ClearMessages();
	void ScrollToEnd();
	
protected:
	bool bIsUserScrolled;
	TSharedPtr<FSUDSTraceLogMarshaller> TraceLogMarshaller;
	TSharedPtr<SMultiLineEditableTextBox> TraceLogTextBox;

	void OnUserScrolled(float X);

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
	void UserEditVariable(const FName& Name, FSUDSValue Value);
	void DeleteVariable(const FName& Name);

protected:
	virtual void OnClose() override;

private:
	USUDSScript* Script = nullptr;
	USUDSDialogue* Dialogue = nullptr;
	float VarColumnWidth = 120;
	float PrefixColumnWidth = 100;
	FName StartLabel = NAME_None;
	bool bResetVarsOnStart = true;
	FDelegateHandle ReimportDelegateHandle;
	// FSUDSEditorDialogueRow needs to held by a TSharedPtr for SListView
	TSharedPtr<SListView<TSharedPtr<FSUDSEditorOutputRow>>> OutputListView;
	TArray<TSharedPtr<FSUDSEditorOutputRow>> OutputRows;
	TSharedPtr<SVerticalBox> ChoicesBox;
	TSharedPtr<SListView<TSharedPtr<FSUDSEditorVariableRow>>> VariablesListView;
	TArray<TSharedPtr<FSUDSEditorVariableRow>> VariableRows;
	TSharedPtr<SSUDSTraceLog> TraceLog;
	TMap<FName, FSUDSValue> ManualOverrideVariables;

	const FSlateColor SpeakerColour = FLinearColor(1.0f, 1.0f, 0.6f, 1.0f);
	const FSlateColor ChoiceColour = FLinearColor(0.4f, 1.0f, 0.4f, 1.0f);
	const FSlateColor EventColour = FLinearColor(0.2f, 0.6f, 1.0f, 1.0f);
	const FSlateColor VarSetColour = FLinearColor(0.8f, 0.6f, 0.9f, 1.0f);
	const FSlateColor VarEditColour = FLinearColor(0.6f, 0.3f, 1.0f, 1.0f);
	const FSlateColor StartColour = FLinearColor(1.0f, 0.5f, 0.0f, 1.0f);
	const FSlateColor FinishColour = FLinearColor(1.0f, 0.3f, 0.3f, 1.0f);
	const FSlateColor SelectColour = FLinearColor(1.0f, 0.0f, 0.5f, 1.0f);
	const FSlateColor RowBgColour1 = FLinearColor(0.15f, 0.15f, 0.15f, 1.0f);
	const FSlateColor RowBgColour2 = FLinearColor(0.3f, 0.3f, 0.3f, 1.0f);

	void ExtendToolbar(FToolBarBuilder& ToolbarBuilder, TWeakPtr<SDockTab> Tab);
	TSharedRef<class SWidget> GetStartLabelMenu();
	FText GetSelectedStartLabel() const;
	void OnStartLabelSelected(FName Label);
	ECheckBoxState GetResetVarsCheckState() const;
	void OnResetVarsCheckStateChanged(ECheckBoxState NewState);
	FReply AddVariableClicked();
	void UpdateVariables();
	void StartDialogue();
	void DestroyDialogue();
	void UpdateOutput();
	void UpdateChoiceButtons();
	void AddOutputRow(const FText& Prefix, const FText& Line, const FSlateColor& PrefixColour, const FSlateColor& LineColour);
	void AddTraceLogRow(const FName& Category, int SourceLineNo, const FString& Message);

	void AddDialogueStep(const FName& Category, int SourceLineNo, const FText& Description, const FText& Prefix);

	void OnDialogueChoice(USUDSDialogue* Dialogue, int ChoiceIndex, int LineNo);
	void OnDialogueEvent(USUDSDialogue* Dialogue, FName EventName, const TArray<FSUDSValue>& Args, int LineNo);
	void OnDialogueFinished(USUDSDialogue* Dialogue);
	void OnDialogueProceeding(USUDSDialogue* Dialogue);
	void OnDialogueStarting(USUDSDialogue* Dialogue, FName LabelName);
	void OnDialogueSpeakerLine(USUDSDialogue* Dialogue, int LineNo);
	void OnDialogueSetVar(USUDSDialogue* Dialogue, FName VariableName, const FSUDSValue& ToValue, const FString& ExpressionStr, int LineNo);
	void OnDialogueUserEditedVar(USUDSDialogue* Dialogue, FName VariableName, const FSUDSValue& ToValue);
	void OnDialogueSelectEval(USUDSDialogue* Dialogue, const FString& ExpressionStr, bool bSuccess, int LineNo);
	
	TSharedRef<ITableRow> OnGenerateRowForOutput(
		TSharedPtr<FSUDSEditorOutputRow> FsudsEditorDialogueRow,
		const TSharedRef<STableViewBase>& TableViewBase);
	TSharedRef<ITableRow> OnGenerateRowForVariable(TSharedPtr<FSUDSEditorVariableRow> Row,
	                                               const TSharedRef<STableViewBase>& Table);
	FSlateColor GetColourForCategory(const FName& Category);
	void OnPostReimport(UObject* Object, bool bSuccess);
	void Clear();
	void WriteBackTextIDs();
	void GenerateVOAssets();

};

#undef LOCTEXT_NAMESPACE