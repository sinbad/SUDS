#include "SUDSEditorToolkit.h"

#include "SUDSDialogue.h"
#include "SUDSLibrary.h"
#include "SUDSScript.h"
#include "Framework/Text/SlateTextRun.h"
#include "Styling/StyleColors.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"

void FSUDSEditorToolkit::InitEditor(const TArray<UObject*>& InObjects)
{
	if (InObjects.Num() > 0)
	{
		Script = Cast<USUDSScript>(InObjects[0]);

		const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("SUDSEditorLayout")
			->AddArea
			(
				FTabManager::NewPrimaryArea()->SetOrientation(Orient_Vertical)
				->Split
				(
					FTabManager::NewSplitter()
					->SetSizeCoefficient(0.6f)
					->SetOrientation(Orient_Horizontal)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.8f)
						->AddTab("SUDSDialogueTab", ETabState::OpenedTab)
					)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.2f)
						->AddTab("SUDSVariablesTab", ETabState::OpenedTab)
					)
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.4f)
					->AddTab("SUDSLogTab", ETabState::OpenedTab)
				)
			);
		FAssetEditorToolkit::InitAssetEditor(EToolkitMode::Standalone, {}, "SUDSEditor", Layout, true, true, InObjects);

	}
}

void FSUDSEditorToolkit::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(INVTEXT("SUDS Editor"));

	InTabManager->RegisterTabSpawner("SUDSDialogueTab", FOnSpawnTab::CreateLambda([=](const FSpawnTabArgs&)
	{
		OutputListView = SNew(SListView<TSharedPtr<FSUDSEditorOutputRow>>)
				.ItemHeight(24)
				.SelectionMode(ESelectionMode::None)
				.ListItemsSource(&OutputRows)
				.OnGenerateRow(this, &FSUDSEditorToolkit::OnGenerateRowForOutput)
				.HeaderRow(
					SNew(SHeaderRow)
					+ SHeaderRow::Column("PrefixHeader")
					.FillSized(PrefixColumnWidth)
					[
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew( STextBlock )
							.Text( INVTEXT("") )
						]
					]
					+ SHeaderRow::Column("LineHeader")
					.FillWidth(1.0f)
					[
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew( STextBlock )
							.Text( INVTEXT("") )
						]
					]
				);


		ChoicesBox = SNew(SVerticalBox);

		VariablesListView = SNew(SListView<TSharedPtr<FSUDSEditorVariableRow>>)
				.ItemHeight(24)
				.SelectionMode(ESelectionMode::None)
				.ListItemsSource(&VariableRows)
				.OnGenerateRow(this, &FSUDSEditorToolkit::OnGenerateRowForVariable)
				.HeaderRow(
					SNew(SHeaderRow)
					+ SHeaderRow::Column("NameHeader")
					.FillWidth(0.4f)
					[
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew( STextBlock )
							.Text( INVTEXT("Name") )
						]
					]
					+ SHeaderRow::Column("ValueHeader")
					.FillWidth(0.6f)
					[
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew( STextBlock )
							.Text( INVTEXT("Value") )
						]
					]

				);

		
		return SNew(SDockTab)
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.FillHeight(1)
			[
				OutputListView.ToSharedRef()
			]
			+SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Left)
			.Padding(30, 15, 30, 15)
			[
				ChoicesBox.ToSharedRef()
			]
		];
	}))
	.SetDisplayName(INVTEXT("Dialogue Output"))
	.SetGroup(WorkspaceMenuCategory.ToSharedRef());

	InTabManager->RegisterTabSpawner("SUDSVariablesTab", FOnSpawnTab::CreateLambda([=](const FSpawnTabArgs&)
	{
		// Possibly use a SPropertyTable with a custom IPropertyTable to implement variable binding
		return SNew(SDockTab)
		[
			VariablesListView.ToSharedRef()
		];
	}))
	.SetDisplayName(INVTEXT("Variables"))
	.SetGroup(WorkspaceMenuCategory.ToSharedRef());


	InTabManager->RegisterTabSpawner("SUDSLogTab", FOnSpawnTab::CreateLambda([=](const FSpawnTabArgs&)
	{
		TraceLogMarshaller = MakeShareable(new FSUDSTraceLogMarshaller());
		TraceLogTextBox = SNew(SMultiLineEditableTextBox)
			.Style(FEditorStyle::Get(), "Log.TextBox")
			.TextStyle(FEditorStyle::Get(), "Log.Normal")
			.Marshaller(TraceLogMarshaller)
			.IsReadOnly(true)
			.AlwaysShowScrollbars(true);
		return SNew(SDockTab)
		[
			TraceLogTextBox.ToSharedRef()
		];
	}))
	.SetDisplayName(INVTEXT("Trace Log"))
	.SetGroup(WorkspaceMenuCategory.ToSharedRef());	


	// Set up the toolbar
	FSUDSToolbarCommands::Register();
	
	const TWeakPtr<FAssetEditorToolkit> WeakToolkit = this->AsShared();

	TSharedRef<FExtender> ToolbarExtender = MakeShareable(new FExtender);

	ToolbarExtender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateRaw(this, &FSUDSEditorToolkit::ExtendToolbar, TWeakPtr<SDockTab>(TabManager->GetOwnerTab())));
	AddToolbarExtender(ToolbarExtender);

	GetToolkitCommands()->MapAction(FSUDSToolbarCommands::Get().StartDialogue,
		FExecuteAction::CreateSP(this, &FSUDSEditorToolkit::StartDialogue),
		FCanExecuteAction());

	//RegenerateMenusAndToolbars();
		
	
}

void FSUDSEditorToolkit::ExtendToolbar(FToolBarBuilder& ToolbarBuilder, TWeakPtr<SDockTab> Tab)
{
	if (!Tab.IsValid())
	{
		return;
	}

	ToolbarBuilder.AddToolBarButton(FSUDSToolbarCommands::Get().StartDialogue,
		NAME_None, TAttribute<FText>(), TAttribute<FText>(),
		FSlateIcon(FEditorStyle::GetStyleSetName(), TEXT("BlueprintMerge.NextDiff")));
}



void FSUDSEditorToolkit::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

	InTabManager->UnregisterTabSpawner("SUDSDialogueTab");
	InTabManager->UnregisterTabSpawner("SUDSVariablesTab");
	InTabManager->UnregisterTabSpawner("SUDSLogTab");
}

FText FSUDSEditorToolkit::GetBaseToolkitName() const
{
	return INVTEXT("SUDS Script Editor");
}

FName FSUDSEditorToolkit::GetToolkitFName() const
{
	return "SUDSScriptEditor";
}

FLinearColor FSUDSEditorToolkit::GetWorldCentricTabColorScale() const
{
	return FLinearColor();
}

FString FSUDSEditorToolkit::GetWorldCentricTabPrefix() const
{
	return "SUDS Script ";
}

void FSUDSEditorToolkit::OnClose()
{
	FAssetEditorToolkit::OnClose();

	if (Dialogue)
	{
		Dialogue->MarkAsGarbage();
		Dialogue = nullptr;
	}
}

void FSUDSEditorToolkit::StartDialogue()
{
	OutputRows.Empty();
	OutputListView->RequestListRefresh();
	VariableRows.Empty();
	TraceLogMarshaller->ClearMessages();
	if (!Dialogue)
	{
		Dialogue = USUDSLibrary::CreateDialogue(nullptr, Script);
		Dialogue->SetFlags(RF_Transient);
		Dialogue->ClearFlags(RF_Transactional);
		
		Dialogue->InternalOnChoice.BindSP(this, &FSUDSEditorToolkit::OnDialogueChoice);
		Dialogue->InternalOnEvent.BindSP(this, &FSUDSEditorToolkit::OnDialogueEvent);
		Dialogue->InternalOnFinished.BindSP(this, &FSUDSEditorToolkit::OnDialogueFinished);
		Dialogue->InternalOnProceeding.BindSP(this, &FSUDSEditorToolkit::OnDialogueProceeding);
		Dialogue->InternalOnStarting.BindSP(this, &FSUDSEditorToolkit::OnDialogueStarting);
		Dialogue->InternalOnSpeakerLine.BindSP(this, &FSUDSEditorToolkit::OnDialogueSpeakerLine);
		Dialogue->InternalOnVariableChanged.BindSP(this, &FSUDSEditorToolkit::OnDialogueVariableChanged);
		Dialogue->InternalOnVariableRequested.BindSP(this, &FSUDSEditorToolkit::OnDialogueVariableRequested);


	}
	Dialogue->Restart(true);

	UpdateVariables();
	
}

void FSUDSEditorToolkit::UpdateOutput()
{
	OutputListView->RequestListRefresh();
	OutputListView->ScrollToBottom();
}

void FSUDSEditorToolkit::UpdateChoiceButtons()
{
	USUDSDialogue* D = Dialogue;
	ChoicesBox->ClearChildren();
	if (!D->IsEnded())
	{
		if (D->IsSimpleContinue())
		{
			ChoicesBox->AddSlot()
			.Padding(0, 0 ,0 , 5)
			[
				SNew(SButton)
				.HAlign(HAlign_Left)
				.Text(INVTEXT("..."))
				.OnClicked_Lambda([D]()
				{
					D->Continue();
					return FReply::Handled();
				})
		];
		}
		else
		{
			for (int i = 0; i < D->GetNumberOfChoices(); ++i)
			{
				ChoicesBox->AddSlot()
				.Padding(0, 0 ,0 , 5)
				[
					SNew(SButton)
					.HAlign(HAlign_Left)
					.Text(D->GetChoiceText(i))
					.OnClicked_Lambda([D, i]()
					{
						D->Choose(i);
						return FReply::Handled();
					})
				];
			}
		
		}
	}
	
}

void FSUDSEditorToolkit::OnDialogueChoice(USUDSDialogue* D, int ChoiceIndex)
{
	if (!D->IsSimpleContinue())
	{
		AddOutputRow(INVTEXT("Choice"),
		             FText::Format(INVTEXT("[{1}] {0}"), D->GetChoiceText(ChoiceIndex), ChoiceIndex),
		             ChoiceColour,
		             ChoiceColour);

	}
}

void FSUDSEditorToolkit::OnDialogueEvent(USUDSDialogue* D, FName EventName, const TArray<FSUDSValue>& Args)
{
	FStringBuilderBase B;
	if (Args.Num() > 0)
	{
		for (auto& Arg : Args)
		{
			if (B.Len() > 0)
			{
				B.Appendf(TEXT(", %s"), *Arg.ToString());
			}
			else
			{
				B.Append("( ");
				B.Append(Arg.ToString());
			}
		}
		B.Append(" )");
	}
	FText ArgText = FText::FromString(B.ToString());
	AddOutputRow(INVTEXT("Event"),
	             FText::FormatOrdered(INVTEXT("{0} {1}"), FText::FromName(EventName), ArgText),
	             EventColour,
	             EventColour);
	
}

void FSUDSEditorToolkit::OnDialogueFinished(USUDSDialogue* D)
{
	AddOutputRow(INVTEXT("End"),
	             INVTEXT("Dialogue Finished"),
	             FinishColour,
	             FinishColour);
}

void FSUDSEditorToolkit::OnDialogueProceeding(USUDSDialogue* D)
{
}

void FSUDSEditorToolkit::OnDialogueStarting(USUDSDialogue* D, FName LabelName)
{
	FString LabelStr = LabelName.IsNone() ? FString("beginning") : LabelName.ToString();
	AddOutputRow(INVTEXT("Start"),
	             FText::Format(INVTEXT("Starting from {0}"), FText::FromString(LabelStr)),
	             StartColour,
	             StartColour);
}

void FSUDSEditorToolkit::OnDialogueSpeakerLine(USUDSDialogue* D)
{
	AddOutputRow(D->GetSpeakerDisplayName(), D->GetText(), SpeakerColour, FSlateColor::UseForeground());
	UpdateChoiceButtons();

}

void FSUDSEditorToolkit::AddOutputRow(const FText& Prefix,
	const FText& Line,
	const FSlateColor& PrefixColour,
	const FSlateColor& LineColour)
{
	// Stupid table changes any colours I give it, white = dark grey
	// Can't figure out where it's coming from, I f**king hate Slate 
	const FSlateColor BgColour = (OutputRows.Num() % 2) > 0 ? FSlateColor(FLinearColor::White) : FSlateColor(FLinearColor(0.9f, 0.9f, 0.9f, 1));
	OutputRows.Add(MakeShareable(
		new FSUDSEditorOutputRow(Prefix, Line, PrefixColour, LineColour, BgColour)));
	UpdateOutput();

	// TEST
	TraceLogMarshaller->AppendMessage(FName(Prefix.ToString()), Line.ToString(), PrefixColour);
}

void FSUDSEditorToolkit::UpdateVariables()
{
	VariableRows.Empty();
	for (auto& Pair : Dialogue->GetVariables())
	{
		VariableRows.Add(MakeShareable(new FSUDSEditorVariableRow(Pair.Key, Pair.Value)));
	}
	
	VariablesListView->RequestListRefresh();
	
}


void FSUDSEditorToolkit::OnDialogueVariableChanged(USUDSDialogue* D,
	FName VariableName,
	const FSUDSValue& ToValue,
	bool bFromScript)
{
	if (bFromScript)
	{
		AddOutputRow(INVTEXT("Var Set"),
		             FText::Format(INVTEXT("{0} = {1}"),
		                           FText::FromName(VariableName),
		                           FText::FromString(ToValue.ToString())),
		             VarSetColour,
		             VarSetColour);
	}
	UpdateVariables();
}

void FSUDSEditorToolkit::OnDialogueVariableRequested(USUDSDialogue* D, FName VariableName)
{
}

TSharedRef<ITableRow> FSUDSEditorToolkit::OnGenerateRowForOutput(
	TSharedPtr<FSUDSEditorOutputRow> Row,
	const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew( SSUDSEditorOutputItem, OwnerTable )
		.Prefix(Row->Prefix)
		.PrefixColour(Row->PrefixColour)
		.Line(Row->Line)
		.LineColour(Row->LineColour)
		.BgColour(Row->BgColour)
		.InitialWidth( PrefixColumnWidth );
}

TSharedRef<ITableRow> FSUDSEditorToolkit::OnGenerateRowForVariable(TSharedPtr<FSUDSEditorVariableRow> Row,
	const TSharedRef<STableViewBase>& Table)
{

	return SNew( SSUDSEditorVariableItem, Table )
		.VariableName(Row->Name)
		.VariableValue(Row->Value)
		.InitialWidth( VarColumnWidth );
	
}

void SSUDSEditorVariableItem::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
{
	InitialWidth = InArgs._InitialWidth;
	VariableName = InArgs._VariableName;
	VariableValue = InArgs._VariableValue;

	SMultiColumnTableRow< TSharedPtr< FString > >::Construct( SMultiColumnTableRow< TSharedPtr< FString > >::FArguments(), InOwnerTableView );
}

TSharedRef<SWidget> SSUDSEditorVariableItem::GenerateWidgetForColumn(const FName& ColumnName)
{
	if (ColumnName == "NameHeader")
	{
		return SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.FillWidth(1.0)
		.Padding(10, 5, 10, 5)
		[
			SNew(STextBlock)
			.Text(FText::FromName(VariableName))
		];
		
	}
	else
	{
		// TODO: editable widgets
		TSharedPtr<SWidget> ValueWidget;

		ValueWidget = SNew(STextBlock)
			.Text(FText::FromString(VariableValue.ToString()));

		/*
		switch (VariableValue.GetType())
		{
		case ESUDSValueType::Int:
			ValueWidget = SNew(STextBlock)
			.Text(FText::FromString(VariableValue.ToString())
			break;
		case ESUDSValueType::Float:
			ValueWidget = SNew(STextBlock)
			.Text(FText::Format(INVTEXT("{0}"), VariableValue.GetFloatValue()));
			break;
		case ESUDSValueType::Boolean:
			ValueWidget = SNew(STextBlock)
			.Text(VariableValue.GetBooleanValue() ? FText::FromString("True") : FText::FromString("False"));
			break;
		case ESUDSValueType::Gender:
			ValueWidget = SNew(STextBlock)
			.Text(FText::Format(INVTEXT("{0}"), VariableValue.GetGenderValue()));
			break;
		case ESUDSValueType::Text:
			ValueWidget = SNew(STextBlock)
			.Text(VariableValue.GetTextValue());
			break;
		case ESUDSValueType::Name:
			ValueWidget = SNew(STextBlock)
			.Text(FText::FromString(VariableValue.GetNameValue().ToString()));
			break;
		case ESUDSValueType::Variable:
		case ESUDSValueType::Empty:
		default:
			ValueWidget = SNew(STextBlock);
			break;
			
		};
		*/
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(10, 5, 10, 5)
			[
				ValueWidget.ToSharedRef()
			];
	}
	
}

void SSUDSEditorOutputItem::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
{
	InitialWidth = InArgs._InitialWidth;
	Prefix = InArgs._Prefix;
	PrefixColour = InArgs._PrefixColour;
	Line = InArgs._Line;
	LineColour = InArgs._LineColour;

	SetBorderBackgroundColor(InArgs._BgColour);

	SMultiColumnTableRow< TSharedPtr< FString > >::Construct( SMultiColumnTableRow< TSharedPtr< FString > >::FArguments(), InOwnerTableView );
}

TSharedRef<SWidget> SSUDSEditorOutputItem::GenerateWidgetForColumn(const FName& ColumnName)
{
	if (ColumnName == "PrefixHeader")
	{
		return SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.FillWidth(1.0)
		.Padding(10, 5, 10, 5)
		[
			SNew(STextBlock)
			.Text(Prefix)
			.ColorAndOpacity(PrefixColour)
		];
	}
	else
	{
		return SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.FillWidth(1.0)
		.Padding(10, 5, 10, 5)
		[
			SNew(STextBlock)
			.Text(Line)
			.ColorAndOpacity(LineColour)
		];
	}

	
}

FSUDSTraceLogMarshaller::FSUDSTraceLogMarshaller()
{
}

void FSUDSTraceLogMarshaller::SetText(const FString& SourceString, FTextLayout& TargetTextLayout)
{
	
	static const FName LogNormalStyle(TEXT("Log.Normal"));
	const FTextBlockStyle& OrigStyle = FEditorStyle::Get().GetWidgetStyle<FTextBlockStyle>(LogNormalStyle);

	for (auto Msg : Messages)
	{
		// Get base style & copy
		FTextBlockStyle Style = OrigStyle;
		Style.ColorAndOpacity = Msg->Colour;
		
		TArray<TSharedRef<IRun>> Runs;
		Runs.Add(FSlateTextRun::Create(FRunInfo(), Msg->Message,  Style));
		
		TargetTextLayout.AddLine(FTextLayout::FNewLineData(Msg->Message, Runs));
	}
	
}

void FSUDSTraceLogMarshaller::GetText(FString& TargetString, const FTextLayout& SourceTextLayout)
{
	SourceTextLayout.GetAsText(TargetString);
}

void FSUDSTraceLogMarshaller::AppendMessage(FName InCategory, const FString& Message, const FSlateColor& Colour)
{
	const FString ConcatLine = FString::Printf(TEXT("%s: %s"), *InCategory.ToString(), *Message);
	Messages.Add(MakeShareable(new FSUDSTraceLogMessage(InCategory, ConcatLine, Colour)));
	MakeDirty();
}

void FSUDSTraceLogMarshaller::ClearMessages()
{
	Messages.Empty();
	MakeDirty();
}
