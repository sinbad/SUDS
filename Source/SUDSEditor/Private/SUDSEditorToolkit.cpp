#include "SUDSEditorToolkit.h"

#include "EditorReimportHandler.h"
#include "SUDSDialogue.h"
#include "SUDSLibrary.h"
#include "SUDSScript.h"
#include "Framework/Text/SlateTextRun.h"
#include "Styling/StyleColors.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"

const FName NAME_SpeakerLine("SpeakerLine");
const FName NAME_Choice("Choice");
const FName NAME_VariableSet("VariableSet");
const FName NAME_SelectEval("Condition");
const FName NAME_Event("Event");
const FName NAME_Start("Start");
const FName NAME_Finish("Finish");


void FSUDSEditorToolkit::InitEditor(const TArray<UObject*>& InObjects)
{
	if (InObjects.Num() > 0)
	{
		Script = Cast<USUDSScript>(InObjects[0]);

		FReimportManager::Instance()->OnPostReimport().AddRaw(this, &FSUDSEditorToolkit::OnPostReimport);		

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
		TraceLog = SNew(SSUDSTraceLog);
		return SNew(SDockTab)
		[
			TraceLog.ToSharedRef()
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

	ToolbarBuilder.BeginSection("CachedState");
	{
		ToolbarBuilder.AddToolBarButton(FSUDSToolbarCommands::Get().StartDialogue,
			NAME_None, TAttribute<FText>(), TAttribute<FText>(),
			FSlateIcon(FEditorStyle::GetStyleSetName(), TEXT("BlueprintMerge.NextDiff")));

		TSharedRef<SWidget> LabelSelectionBox = SNew(SComboButton)
			.OnGetMenuContent(this, &FSUDSEditorToolkit::GetStartLabelMenu)
			.ButtonContent()
			[
				SNew(STextBlock)
				.ToolTipText( INVTEXT("Choose where to start dialogue from") )
				.Text(this, &FSUDSEditorToolkit::GetSelectedStartLabel )
			];

		ToolbarBuilder.AddWidget(LabelSelectionBox);
		
	}
	ToolbarBuilder.EndSection();
	
}

TSharedRef<SWidget> FSUDSEditorToolkit::GetStartLabelMenu()
{
	FMenuBuilder MenuBuilder(true, NULL);

	const FUIAction StartAction(FExecuteAction::CreateSP(this, &FSUDSEditorToolkit::OnStartLabelSelected, FName(NAME_None)));
	MenuBuilder.AddMenuEntry(INVTEXT("Beginning"), INVTEXT("Start from the beginning of the dialogue"), FSlateIcon(), StartAction);
	MenuBuilder.AddSeparator();
	
	if (IsValid(Script))
	{
		for (auto& LabelPair : Script->GetLabelList())
		{
			FUIAction LabelAction(FExecuteAction::CreateSP(this, &FSUDSEditorToolkit::OnStartLabelSelected, LabelPair.Key));
			MenuBuilder.AddMenuEntry(FText::FromName(LabelPair.Key), FText(), FSlateIcon(), LabelAction);
		}
	}


	return MenuBuilder.MakeWidget();
}

void FSUDSEditorToolkit::OnStartLabelSelected(FName Label)
{
	StartLabel = Label;
}

FText FSUDSEditorToolkit::GetSelectedStartLabel() const
{
	return FText::Format(INVTEXT("From: {0}"),
		StartLabel.IsNone() ? INVTEXT("Beginning") : FText::FromName(StartLabel));
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
	DestroyDialogue();

}

void FSUDSEditorToolkit::StartDialogue()
{
	Clear();
	if (!Dialogue)
	{
		// Custom creation of USUDSDialogue, we want it to exist only for this window.
		// Mark as a root object (we'll clear it on close)
		{
			// Have to guard vs GC
			FGCScopeGuard GCGuard;
			const FName Name = MakeUniqueObjectName(GetTransientPackage(),
			                                        USUDSDialogue::StaticClass(),
			                                        Script->GetFName());
			Dialogue = NewObject<USUDSDialogue>(GetTransientPackage(),
				Name, RF_Transient | RF_MarkAsRootSet);
		}
		Dialogue->Initialise(Script);
		
		Dialogue->InternalOnChoice.BindSP(this, &FSUDSEditorToolkit::OnDialogueChoice);
		Dialogue->InternalOnEvent.BindSP(this, &FSUDSEditorToolkit::OnDialogueEvent);
		Dialogue->InternalOnFinished.BindSP(this, &FSUDSEditorToolkit::OnDialogueFinished);
		Dialogue->InternalOnProceeding.BindSP(this, &FSUDSEditorToolkit::OnDialogueProceeding);
		Dialogue->InternalOnStarting.BindSP(this, &FSUDSEditorToolkit::OnDialogueStarting);
		Dialogue->InternalOnSpeakerLine.BindSP(this, &FSUDSEditorToolkit::OnDialogueSpeakerLine);
		Dialogue->InternalOnSetVar.BindSP(this, &FSUDSEditorToolkit::OnDialogueSetVar);
		Dialogue->InternalOnSelectEval.BindSP(this, &FSUDSEditorToolkit::OnDialogueSelectEval);


	}
	Dialogue->Restart(true, StartLabel);

	UpdateVariables();
	
}

void FSUDSEditorToolkit::Clear()
{
	OutputRows.Empty();
	OutputListView->RequestListRefresh();
	VariableRows.Empty();
	VariablesListView->RequestListRefresh();
	TraceLog->ClearMessages();
	
}

void FSUDSEditorToolkit::DestroyDialogue()
{
	if (Dialogue)
	{
		// Handle garbage collection of our UObject
		{
			FGCScopeGuard GCGuard;
			Dialogue->RemoveFromRoot();
			Dialogue->MarkAsGarbage();
		}
		Dialogue = nullptr;
		CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	}
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
				.Text(INVTEXT("(Continue...)"))
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

void FSUDSEditorToolkit::OnDialogueChoice(USUDSDialogue* D, int ChoiceIndex, int LineNo)
{
	if (!D->IsSimpleContinue())
	{
		AddDialogueStep(NAME_Choice, LineNo,
		                FText::Format(INVTEXT("[{1}] {0}"), D->GetChoiceText(ChoiceIndex), ChoiceIndex),
		                INVTEXT("Choice"));

	}
}

void FSUDSEditorToolkit::OnDialogueEvent(USUDSDialogue* D, FName EventName, const TArray<FSUDSValue>& Args, int LineNo)
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
	AddDialogueStep(NAME_Event, LineNo,
	                FText::FormatOrdered(INVTEXT("{0} {1}"), FText::FromName(EventName), ArgText),
	                INVTEXT("Event"));
	
}

void FSUDSEditorToolkit::OnDialogueFinished(USUDSDialogue* D)
{
	AddDialogueStep(NAME_Finish, 0,
	                INVTEXT("Dialogue Finished"),
	                INVTEXT("Finish"));
	UpdateChoiceButtons();
}

void FSUDSEditorToolkit::OnDialogueProceeding(USUDSDialogue* D)
{
}

void FSUDSEditorToolkit::OnDialogueStarting(USUDSDialogue* D, FName LabelName)
{
	FString LabelStr = LabelName.IsNone() ? FString("beginning") : LabelName.ToString();
	AddDialogueStep(NAME_Start, 0, 
	                FText::Format(INVTEXT("Starting from {0}"), FText::FromString(LabelStr)),
	                INVTEXT("Start"));
}

void FSUDSEditorToolkit::OnDialogueSpeakerLine(USUDSDialogue* D, int LineNo)
{
	AddDialogueStep(NAME_SpeakerLine, LineNo , D->GetText(),D->GetSpeakerDisplayName());
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

}

void FSUDSEditorToolkit::AddTraceLogRow(const FName& Category, int SourceLineNo, const FString& Message)
{
	FSlateColor Colour = GetColourForCategory(Category);
	
	TraceLog->AppendMessage(Category, SourceLineNo, Message, Colour);
}

FSlateColor FSUDSEditorToolkit::GetColourForCategory(const FName& Category)
{
	if (Category == NAME_SpeakerLine)
	{
		return SpeakerColour;
	}
	else if (Category == NAME_Choice)
	{
		return ChoiceColour;
	}
	else if (Category == NAME_Event)
	{
		return EventColour;
	}
	else if (Category == NAME_VariableSet)
	{
		return VarSetColour;
	}
	else if (Category == NAME_Start)
	{
		return StartColour;
	}
	else if (Category == NAME_Finish)
	{
		return FinishColour;
	}
	else if (Category == NAME_SelectEval)
	{
		return SelectColour;
	}

	return FSlateColor::UseForeground();
	
}

void FSUDSEditorToolkit::AddDialogueStep(const FName& Category, int SourceLineNo, const FText& Description, const FText& Prefix)
{
	if (Category == NAME_SpeakerLine)
	{
		AddOutputRow(Prefix, Description, SpeakerColour, FSlateColor::UseForeground());
		AddTraceLogRow(Category, SourceLineNo, FString::Printf(TEXT("%s: %s"), *Prefix.ToString(), *Description.ToString()));
	}
	else if (Category == NAME_Choice)
	{
		AddOutputRow(Prefix, Description, ChoiceColour, ChoiceColour);
		AddTraceLogRow(Category, SourceLineNo, Description.ToString());
	}
	else if (Category == NAME_Start)
	{
		AddOutputRow(Prefix, Description, StartColour, StartColour);
		AddTraceLogRow(Category, SourceLineNo, Description.ToString());
	}
	else if (Category == NAME_Finish)
	{
		AddOutputRow(Prefix, Description, FinishColour, FinishColour);
		AddTraceLogRow(Category, SourceLineNo, Description.ToString());
	}
	else
	{
		AddTraceLogRow(Category, SourceLineNo, Description.ToString());
	}
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


void FSUDSEditorToolkit::OnPostReimport(UObject* Object, bool bSuccess)
{
	if (Object == Script)
	{
		// Destroy any dialogue instance, will not be valid post-import
		DestroyDialogue();
		Clear();
	}
}

void FSUDSEditorToolkit::OnDialogueSetVar(USUDSDialogue* D,
	FName VariableName,
	const FSUDSValue& ToValue,
	const FString& ExpressionStr,
	int LineNo)
{
	FText Description;
	if (ExpressionStr.Len() > 0)
	{
		Description = FText::Format(INVTEXT("{0} = {1} = {2}"),
		                            FText::FromName(VariableName),
		                            FText::FromString(ExpressionStr),
		                            FText::FromString(ToValue.ToString()));
	}
	else
	{
		Description = FText::Format(INVTEXT("{0} = {1}"),
									FText::FromName(VariableName),
									FText::FromString(ToValue.ToString()));
		
	}
	AddDialogueStep(NAME_VariableSet,
	                LineNo,
	                Description,
	                INVTEXT("Set Variable"));
	UpdateVariables();
}

void FSUDSEditorToolkit::OnDialogueSelectEval(USUDSDialogue* D,
	const FString& ExpressionStr,
	bool bSuccess,
	int LineNo)
{
	AddDialogueStep(NAME_SelectEval,
	                LineNo,
	                FText::Format(INVTEXT("{0} = {1}"),
	                              FText::FromString(ExpressionStr),
	                              bSuccess ? INVTEXT("true") : INVTEXT("false")),
	                INVTEXT("Conditional"));
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
			.AutoWrapText(true)
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

void FSUDSTraceLogMarshaller::AppendMessage(FName InCategory, int LineNo, const FString& Message, const FSlateColor& Colour)
{
	const int MinCategorySize = 12;
	const FString CatStr = InCategory.ToString();
	int CatLen = CatStr.Len();
	const FString CatPadding = CatLen < MinCategorySize ? FString::ChrN(MinCategorySize - CatLen, ' ') : "";
	const FString ConcatLine = FString::Printf(TEXT("%s[%s] L%04d %s"), *CatPadding, *CatStr, LineNo, *Message);
	Messages.Add(MakeShareable(new FSUDSTraceLogMessage(InCategory, ConcatLine, Colour)));
	MakeDirty();
}

void FSUDSTraceLogMarshaller::ClearMessages()
{
	Messages.Empty();
	MakeDirty();
}

void SSUDSTraceLog::Construct(const FArguments& InArgs)
{
	TraceLogMarshaller = MakeShareable(new FSUDSTraceLogMarshaller());
	TraceLogTextBox = SNew(SMultiLineEditableTextBox)
		.Style(FEditorStyle::Get(), "Log.TextBox")
		.TextStyle(FEditorStyle::Get(), "Log.Normal")
		.Marshaller(TraceLogMarshaller)
		.IsReadOnly(true)
		.AutoWrapText(true)
		.OnVScrollBarUserScrolled(this, &SSUDSTraceLog::OnUserScrolled)
		.AlwaysShowScrollbars(true);

	ChildSlot
	.Padding(3)
	[
		SNew(SVerticalBox)

		// could add a filter here like output log
		
		// log area
		+SVerticalBox::Slot()
		.FillHeight(1)
		[
			TraceLogTextBox.ToSharedRef()
		]

	];
	
	
}

void SSUDSTraceLog::OnUserScrolled(float X)
{
	// revert to auto scroll when near the bottom
	bIsUserScrolled = X < (1.0 - SMALL_NUMBER);
}

void SSUDSTraceLog::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	// We seem to have to do this in Tick(), trying to do it after appending never works
	if (!bIsUserScrolled)
	{
		ScrollToEnd();
	}
}

void SSUDSTraceLog::AppendMessage(FName InCategory, int LineNo, const FString& Message, const FSlateColor& Colour)
{
	TraceLogMarshaller->AppendMessage(InCategory, LineNo, Message, Colour);
}

void SSUDSTraceLog::ClearMessages()
{
	TraceLogMarshaller->ClearMessages();
}

void SSUDSTraceLog::ScrollToEnd()
{
	TraceLogTextBox->ScrollTo(ETextLocation::EndOfDocument);
	bIsUserScrolled = false;
}
