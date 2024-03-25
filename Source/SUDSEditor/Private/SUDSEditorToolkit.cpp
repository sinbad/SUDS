// Copyright Steve Streeting 2022
// Released under the MIT license https://opensource.org/license/MIT/
#include "SUDSEditorToolkit.h"

#include "EditorReimportHandler.h"
#include "IDetailRootObjectCustomization.h"
#include "IDocumentation.h"
#include "SUDSDialogue.h"
#include "SUDSEditorScriptTools.h"
#include "SUDSEditorVoiceOverTools.h"
#include "SUDSLibrary.h"
#include "SUDSMessageLogger.h"
#include "SUDSScript.h"
#include "SUDSScriptNodeText.h"
#include "SUDSSubsystem.h"
#include "Framework/Text/SlateTextRun.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Input/SNumericEntryBox.h"

const FName NAME_SpeakerLine("SpeakerLine");
const FName NAME_Choice("Choice");
const FName NAME_VariableSet("VariableSet");
const FName NAME_VariableEdit("VariableEdit");
const FName NAME_SelectEval("Condition");
const FName NAME_Event("Event");
const FName NAME_Start("Start");
const FName NAME_Finish("Finish");


class FSUDSDetailRootObjectCustomization : public  IDetailRootObjectCustomization
{
public:
	virtual TSharedPtr<SWidget> CustomizeObjectHeader(const FDetailsObjectSet& InRootObjectSet,
		const TSharedPtr<ITableRow>& InTableRow) override
	{
		auto Ret = SNew(STextBlock)
		.Margin(FMargin(10,5))
		.TextStyle(&FAppStyle::Get().GetWidgetStyle<FTextBlockStyle>("SmallText"));
		if (auto TN = Cast<USUDSScriptNodeText>(InRootObjectSet.RootObjects[0]))
		{
			Ret->SetText(FText::FromString(FString::Printf(TEXT("Speaker Line - ID: %s  Source Line: %d"), *TN->GetTextID(), TN->GetSourceLineNo())));
		}
		return Ret;
	}

	virtual bool ShouldDisplayHeader(const FDetailsObjectSet& InRootObjectSet) const override
	{
		return InRootObjectSet.RootObjects[0]->IsA(USUDSScriptNodeText::StaticClass());
	}
};

void FSUDSEditorToolkit::InitEditor(const TArray<UObject*>& InObjects)
{
	if (InObjects.Num() > 0)
	{
		Script = Cast<USUDSScript>(InObjects[0]);

		ReimportDelegateHandle = FReimportManager::Instance()->OnPostReimport().AddRaw(this, &FSUDSEditorToolkit::OnPostReimport);		

		const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("SUDSEditorLayout_v3")
			->AddArea
			(
				FTabManager::NewPrimaryArea()->SetOrientation(Orient_Horizontal)
				->Split
				(
					FTabManager::NewSplitter()
					->SetSizeCoefficient(0.75f)
					->SetOrientation(Orient_Horizontal)
					->Split
					(
						FTabManager::NewSplitter()
						->SetSizeCoefficient(0.6f)
						->SetOrientation(Orient_Vertical)
						->Split
						(

						
							FTabManager::NewStack()
							->SetSizeCoefficient(0.8f)
							->AddTab("SUDSDialogueTab", ETabState::OpenedTab)
						)
						->Split
						(
							FTabManager::NewStack()
							->SetSizeCoefficient(0.4f)
							->AddTab("SUDSLogTab", ETabState::OpenedTab)
						)
					)
					->Split
					(
						FTabManager::NewSplitter()
						->SetSizeCoefficient(0.25f)
						->SetOrientation(Orient_Vertical)
						->Split
						(
							FTabManager::NewStack()
							->SetSizeCoefficient(0.6f)
							->AddTab("SUDSVariablesTab", ETabState::OpenedTab)
						)
						->Split
						(
							FTabManager::NewStack()
							->SetSizeCoefficient(0.4f)
							->AddTab("SUDSDetailsTab", ETabState::OpenedTab)
						)
					)
				)
			);
		FAssetEditorToolkit::InitAssetEditor(EToolkitMode::Standalone, {}, "SUDSEditor", Layout, true, true, InObjects);

	}
}

void FSUDSEditorToolkit::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(INVTEXT("SUDS Editor"));

	InTabManager->RegisterTabSpawner("SUDSDialogueTab", FOnSpawnTab::CreateLambda([this](const FSpawnTabArgs&)
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
				.ItemHeight(28)
				.SelectionMode(ESelectionMode::None)
				.ListItemsSource(&VariableRows)
				.OnGenerateRow(this, &FSUDSEditorToolkit::OnGenerateRowForVariable)
				.HeaderRow(
					SNew(SHeaderRow)
					+ SHeaderRow::Column("NameHeader")
					.FillSized(VarColumnWidth)
					.VAlignCell(VAlign_Center)
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
					.FillWidth(1.0f)
					.VAlignCell(VAlign_Fill)
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

		// To ensure globals are pre-populated
		UpdateVariables();
		
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

	InTabManager->RegisterTabSpawner("SUDSVariablesTab", FOnSpawnTab::CreateLambda([this](const FSpawnTabArgs&)
	{
		// Possibly use a SPropertyTable with a custom IPropertyTable to implement variable binding
		return SNew(SDockTab)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.VAlign(VAlign_Top)
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Right)
				.Padding(2)
				[
					SNew(SButton)
					.Text(INVTEXT("Add Variable"))
					.OnClicked(this, &FSUDSEditorToolkit::AddVariableClicked)
				]
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0)
			[
				VariablesListView.ToSharedRef()
			]
		];
	}))
	.SetDisplayName(INVTEXT("Variables"))
	.SetGroup(WorkspaceMenuCategory.ToSharedRef());

	InTabManager->RegisterTabSpawner("SUDSDetailsTab", FOnSpawnTab::CreateLambda([this](const FSpawnTabArgs&)
	{
		FPropertyEditorModule& PropertyEditorModule = FModuleManager::Get().LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		FDetailsViewArgs DetailsViewArgs;
		DetailsViewArgs.bShowOptions = false;
		DetailsViewArgs.bUpdatesFromSelection = false;
		DetailsViewArgs.bShowPropertyMatrixButton = false;
		DetailsViewArgs.bAllowSearch = false;
		DetailsViewArgs.bAllowMultipleTopLevelObjects = true;
		DetailsViewArgs.bShowObjectLabel = false;
		DetailsViewArgs.bHideSelectionTip = true;
		auto DetailView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
		DetailView->SetRootObjectCustomizationInstance(MakeShareable(new FSUDSDetailRootObjectCustomization()));
		TArray<UObject*> ObjectsInDetailView;
		ObjectsInDetailView.Add(Script);
		for (auto Node : Script->GetNodes())
		{
			if (auto TNode = Cast<USUDSScriptNodeText>(Node))
			{
				ObjectsInDetailView.Add(TNode);
			}
		}
		DetailView->SetObjects(ObjectsInDetailView, true, true);
		// Possibly use a SPropertyTable with a custom IPropertyTable to implement variable binding
		return SNew(SDockTab)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.VAlign(VAlign_Top)
			[
				DetailView
			]
		];
	}))
	.SetDisplayName(INVTEXT("Details"))
	.SetGroup(WorkspaceMenuCategory.ToSharedRef());
	

	InTabManager->RegisterTabSpawner("SUDSLogTab", FOnSpawnTab::CreateLambda([this](const FSpawnTabArgs&)
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
	GetToolkitCommands()->MapAction(FSUDSToolbarCommands::Get().WriteBackTextIDs,
		FExecuteAction::CreateSP(this, &FSUDSEditorToolkit::WriteBackTextIDs),
		FCanExecuteAction());
	GetToolkitCommands()->MapAction(FSUDSToolbarCommands::Get().GenerateVOAssets,
		FExecuteAction::CreateSP(this, &FSUDSEditorToolkit::GenerateVOAssets),
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
			FSlateIcon(
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION > 0
				FAppStyle::GetAppStyleSetName(),
#else
				FEditorStyle::GetStyleSetName(),
#endif
				TEXT("Icons.Toolbar.Play")));

		TSharedRef<SWidget> LabelSelectionBox = SNew(SComboButton)
			.OnGetMenuContent(this, &FSUDSEditorToolkit::GetStartLabelMenu)
			.ButtonContent()
			[
				SNew(STextBlock)
				.ToolTipText( INVTEXT("Choose where to start dialogue from") )
				.Text(this, &FSUDSEditorToolkit::GetSelectedStartLabel )
			];

		ToolbarBuilder.AddWidget(LabelSelectionBox);

		ToolbarBuilder.AddWidget(SNew(STextBlock)
			.Text(FText::FromString(TEXT("Reset Variables:")))
			.Margin(FMargin(10, 7, 10, 5)));
		TSharedPtr<SToolTip> ResetTooltip = IDocumentation::Get()->CreateToolTip(
			FText::FromString(TEXT(
				"Whether to reset variable state created  by the script (note: vars manually created in this editor are never reset, delete them yourself)")),
			NULL,
			"",
			TEXT("ResetVars"));
		TSharedRef<SWidget> ResetVarsCheckbox = SNew(SCheckBox)
			.ToolTip(ResetTooltip)
			.IsChecked(this, &FSUDSEditorToolkit::GetResetVarsCheckState)
			.OnCheckStateChanged(this, &FSUDSEditorToolkit::OnResetVarsCheckStateChanged);
		ToolbarBuilder.AddWidget(ResetVarsCheckbox);
		
	}
	ToolbarBuilder.EndSection();
	
	ToolbarBuilder.AddSeparator();

	ToolbarBuilder.BeginSection("CachedState");
	{
		ToolbarBuilder.AddToolBarButton(FSUDSToolbarCommands::Get().WriteBackTextIDs,
			NAME_None, TAttribute<FText>(), TAttribute<FText>(),
			FSlateIcon(
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION > 0
				FAppStyle::GetAppStyleSetName(),
#else
				FEditorStyle::GetStyleSetName(),
#endif
				TEXT("Icons.Toolbar.Details")));

		ToolbarBuilder.AddToolBarButton(FSUDSToolbarCommands::Get().GenerateVOAssets,
			NAME_None, TAttribute<FText>(), TAttribute<FText>(),
			FSlateIcon(
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION > 0
				FAppStyle::GetAppStyleSetName(),
#else
				FEditorStyle::GetStyleSetName(),
#endif
				TEXT("Icons.Toolbar.Export")));
		
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

ECheckBoxState FSUDSEditorToolkit::GetResetVarsCheckState() const
{
	return bResetVarsOnStart ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void FSUDSEditorToolkit::OnResetVarsCheckStateChanged(ECheckBoxState NewState)
{
	bResetVarsOnStart = NewState == ECheckBoxState::Checked;
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

void FSUDSEditorToolkit::UserEditVariable(const FName& Name, FSUDSValue Value)
{
	FName GlobalName;
	if (USUDSLibrary::IsDialogueVariableGlobal(Name, GlobalName))
	{
		USUDSSubsystem::Test_DummyGlobalVariables.Add(GlobalName, Value);
		OnDialogueUserEditedVar(nullptr, Name, Value);
	}
	else
	{
		// Update manual overrides if it's one of these
		if (ManualOverrideVariables.Contains(Name))
		{
			ManualOverrideVariables[Name] = Value;
		}

		// This will cause a refresh
		if (Dialogue)
			Dialogue->SetVariable(Name, Value);
		else
		{
			// This won't get called if dialogue is not initialised, so call it to get log & refresh
			OnDialogueUserEditedVar(nullptr, Name, Value);
		}
	}
}

void FSUDSEditorToolkit::DeleteVariable(const FName& Name)
{
	FName GlobalName;
	if (USUDSLibrary::IsDialogueVariableGlobal(Name, GlobalName))
	{
		USUDSSubsystem::Test_DummyGlobalVariables.Remove(GlobalName);
	}
	else
	{
		ManualOverrideVariables.Remove(Name);
		if (Dialogue)
			Dialogue->UnSetVariable(Name);
	}
	
	UpdateVariables();
}

void FSUDSEditorToolkit::OnClose()
{
	FAssetEditorToolkit::OnClose();
	FReimportManager::Instance()->OnPostReimport().Remove(ReimportDelegateHandle);
	ReimportDelegateHandle.Reset();

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
		Dialogue->InternalOnSetVarByCode.BindSP(this, &FSUDSEditorToolkit::OnDialogueUserEditedVar);
		Dialogue->InternalOnSelectEval.BindSP(this, &FSUDSEditorToolkit::OnDialogueSelectEval);


	}
	else
	{
		// Reset things manually so we can set manual override vars
		if (bResetVarsOnStart)
			Dialogue->ResetState();
	}
	
	// Set manual override vars before we start
	for (auto& Pair : ManualOverrideVariables)
	{
		Dialogue->SetVariable(Pair.Key, Pair.Value);
	}
	
	Dialogue->Restart(false, StartLabel);


	UpdateVariables();
	
}

void FSUDSEditorToolkit::Clear()
{
	OutputRows.Empty();
	OutputListView->RequestListRefresh();
	VariableRows.Empty();
	VariablesListView->RequestListRefresh();
	//TraceLog->ClearMessages();
	
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
	else if (Category == NAME_VariableEdit)
	{
		return VarEditColour;
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

FReply FSUDSEditorToolkit::AddVariableClicked()
{
	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(INVTEXT("Add Variable"))
		.IsPopupWindow(true)
		.SupportsMaximize(false) 
		.SupportsMinimize(false)
		.CreateTitleBar(true)
		.SizingRule(ESizingRule::Autosized)
		.FocusWhenFirstShown(true)
		.MinWidth(400)
		.ActivationPolicy(EWindowActivationPolicy::FirstShown);

	bool bConfirmed = false;
	ESUDSValueType ValType = ESUDSValueType::Int;
	FText VarNameText = FText::FromString("NewVar");
	
	Window->SetContent(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5)
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.FillWidth(0.4)
			[
				SNew(STextBlock)
				.Text(INVTEXT("Name"))
			]
			+SHorizontalBox::Slot()
			.FillWidth(0.6)
			[
				SNew(SEditableTextBox)
				.MinDesiredWidth(150)
				.SelectAllTextWhenFocused(true)
				.Text(VarNameText)
				.OnTextCommitted_Lambda([&VarNameText](const FText& NewText, ETextCommit::Type CommitType)
				{
					VarNameText = NewText;
				})
			]
			
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5)
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.FillWidth(0.4)
			[
				SNew(STextBlock)
				.Text(INVTEXT("Type"))
			]
			+SHorizontalBox::Slot()
			.FillWidth(0.6)
			[
			SNew(SComboButton)
				.OnGetMenuContent_Lambda([&ValType]
				{
					FMenuBuilder MenuBuilder(true, NULL);

					// NumEnums() - 1 because the last value is the autogen _MAX value
					for (int i = 0; i < StaticEnum<ESUDSValueType>()->NumEnums() - 1; ++i)
					{
						ESUDSValueType Val = (ESUDSValueType)StaticEnum<ESUDSValueType>()->GetValueByIndex(i);
						if (Val != ESUDSValueType::Empty && Val != ESUDSValueType::Variable)
						{
							const FUIAction Action(FExecuteAction::CreateLambda([Val, &ValType]()
							{
								ValType = Val;
							}));
							MenuBuilder.AddMenuEntry(FText::FromString(StaticEnum<ESUDSValueType>()->GetNameStringByIndex(i)),
													 FText(),
													 FSlateIcon(),
													 Action);
						}
		
					}
	
					return MenuBuilder.MakeWidget();
					
				})
				.ButtonContent()
				[
					SNew(STextBlock)
						.Text_Lambda([&ValType]
						{
							return FText::FromString(StaticEnum<ESUDSValueType>()->GetNameStringByValue((int64)ValType));
						})
				]
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 5, 0, 3)
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.HAlign(HAlign_Right)
			.Padding(2)
			[
				SNew(SButton)
				.Text(INVTEXT("OK"))
				.OnClicked_Lambda([&bConfirmed, Window, VarNameText]()
				{
					if (!VarNameText.IsEmptyOrWhitespace())
					{
						bConfirmed = true;
						Window->RequestDestroyWindow();
					}
					return FReply::Handled();
				})
			]
			+SHorizontalBox::Slot()
			.HAlign(HAlign_Right)
			.AutoWidth()
			.Padding(2)
			[
				SNew(SButton)
				.Text(INVTEXT("Cancel"))
				.OnClicked_Lambda([&bConfirmed, Window]()
				{
					bConfirmed = false;
					Window->RequestDestroyWindow();
					return FReply::Handled();
				})
			]
			
		]
	);

	FVector2D Pos = FSlateApplication::Get().GetCursorPos();
	Pos.X -= 230;
	Pos.Y += 100;
	Window->MoveWindowTo(Pos);

	// This doesn't return until closed
	GEditor->EditorAddModalWindow(Window);

	if (bConfirmed)
	{
		FName VarName(VarNameText.ToString().TrimStartAndEnd());
		const FSUDSValue Val(ValType);
		FName GlobalVarName;
		if (USUDSLibrary::IsDialogueVariableGlobal(VarName, GlobalVarName))
		{
			USUDSSubsystem::Test_DummyGlobalVariables.Add(GlobalVarName, Val);
			OnDialogueUserEditedVar(nullptr, VarName, Val);
		}
		else
		{
			ManualOverrideVariables.Add(VarName, Val);
			if (Dialogue)
			{
				// This will cause a refresh
				Dialogue->SetVariable(VarName, Val);
			}
			else
			{
				// This won't get called if dialogue is not initialised, so call it to get log & refresh
				OnDialogueUserEditedVar(nullptr, VarName, Val);
			}
		}
		
	}
	return FReply::Handled();
	
}



void FSUDSEditorToolkit::UpdateVariables()
{
	VariableRows.Empty();
	// From dialogue (will contain manual overrides already)
	if (Dialogue)
	{
		for (auto& Pair : Dialogue->GetVariables())
		{
			VariableRows.Add(MakeShareable(
				new FSUDSEditorVariableRow(Pair.Key, Pair.Value, ManualOverrideVariables.Contains(Pair.Key))));
		}
	}
	else
	{
		// Manual overrides can occur before dialogue is started
		// Otherwise, they will be in the dialogue
		for (auto& Pair : ManualOverrideVariables)
		{
			VariableRows.Add(MakeShareable(new FSUDSEditorVariableRow(Pair.Key, Pair.Value, true)));
		}
	}
	// Also do global vars
	for (auto& Pair : USUDSSubsystem::Test_DummyGlobalVariables)
	{
		VariableRows.Add(MakeShareable(
			new FSUDSEditorVariableRow(
				// We need to insert the "global." prefix back in since it's removed in the global var list
				FName(FString::Printf(TEXT("global.%s"), *Pair.Key.ToString())),
				Pair.Value,
				true)));
	}
	
	VariableRows.Sort();
	
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

void FSUDSEditorToolkit::OnDialogueUserEditedVar(USUDSDialogue* D, FName VariableName, const FSUDSValue& ToValue)
{
	// User-edited value
	const FText Description = FText::Format(INVTEXT("{0} = {1}"),
								FText::FromName(VariableName),
								FText::FromString(ToValue.ToString()));
	AddDialogueStep(NAME_VariableEdit,
					0,
					Description,
					INVTEXT("Edit Variable"));
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
		.bIsManualOverride(Row->bIsManualOverride)
		.Parent(this)
		.InitialWidth( VarColumnWidth );
		
	
}

void FSUDSEditorToolkit::WriteBackTextIDs()
{
	if (FMessageDialog::Open(EAppMsgType::YesNo,
	                         FText::FromString(
		                         "Are you sure you want to write string keys back to this script?"))
		== EAppReturnType::Yes)
	{
		FSUDSMessageLogger::ClearMessages();
		FSUDSMessageLogger Logger;
		FSUDSEditorScriptTools::WriteBackTextIDs(Script, Logger);
	}

}

void FSUDSEditorToolkit::GenerateVOAssets()
{
	if (FMessageDialog::Open(EAppMsgType::YesNo,
	                         FText::FromString(
		                         "Are you sure you want to generate Dialogue Voice / Dialogue Wave assets for this script?"))
		== EAppReturnType::Yes)
	{
		EObjectFlags Flags = RF_Public | RF_Standalone | RF_Transactional;
		FSUDSMessageLogger::ClearMessages();
		FSUDSMessageLogger Logger;
		FSUDSEditorVoiceOverTools::GenerateAssets(Script, Flags, &Logger);
	}
	
}

void SSUDSEditorVariableItem::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
{
	InitialWidth = InArgs._InitialWidth;
	VariableName = InArgs._VariableName;
	VariableValue = InArgs._VariableValue;
	bIsManualOverride = InArgs._bIsManualOverride;
	Parent = InArgs._Parent;

	SMultiColumnTableRow< TSharedPtr< FString > >::Construct( SMultiColumnTableRow< TSharedPtr< FString > >::FArguments(), InOwnerTableView );
}

TSharedRef<SWidget> SSUDSEditorVariableItem::GenerateWidgetForColumn(const FName& ColumnName)
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION > 0	
	const FSlateFontInfo PropertyFont = FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont"));
	const FSlateBrush* BorderBrush = FAppStyle::GetBrush("ToolPanel.GroupBorder");
#else
	const FSlateFontInfo PropertyFont = FEditorStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont"));
	const FSlateBrush* BorderBrush = FEditorStyle::GetBrush("ToolPanel.GroupBorder");
#endif
	const FSlateColor NormalBgColour = FSlateColor(FLinearColor::White);
	const FSlateColor ManualOverrideBgColour = FSlateColor(FLinearColor(0.9f,0.7f,1,1));
	
	if (ColumnName == "NameHeader")
	{
		const FMargin BoxMargin(0, 2, 0, 1.333f);
		return SNew(SBorder)
		.BorderImage(BorderBrush)
		.BorderBackgroundColor(bIsManualOverride ? ManualOverrideBgColour : NormalBgColour)
		.Padding(BoxMargin)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.FillWidth(1.0)
			.Padding(10, 5, 5, 5)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Font(PropertyFont)
				.Text(FText::FromName(VariableName))
			]
		];
		
	}
	else
	{
		TSharedPtr<SWidget> ValueWidget;

		FMargin InnerPadding = FMargin(5, 2, 5, 2);
		switch (VariableValue.GetType())
		{
		case ESUDSValueType::Int:
			ValueWidget = SNew(SNumericEntryBox<int32>)
				.Value(VariableValue.GetIntValue())
				.OnValueCommitted_Lambda([this] (const int32 InValue, ETextCommit::Type)
				{
					Parent->UserEditVariable(VariableName, InValue);
				})
				.MinDesiredValueWidth(60)
				.Font(PropertyFont);
			break;
		case ESUDSValueType::Float:
			ValueWidget = SNew(SNumericEntryBox<float>)
				.Value(VariableValue.GetFloatValue())
				.OnValueCommitted_Lambda([this] (const float InValue, ETextCommit::Type)
				{
					Parent->UserEditVariable(VariableName, InValue);
				})
				.MinDesiredValueWidth(60)
				.Font(PropertyFont);
			break;
		case ESUDSValueType::Boolean:
			ValueWidget = SNew(SCheckBox)
				.IsChecked(VariableValue.GetBooleanValue())
				.OnCheckStateChanged_Lambda([this] (ECheckBoxState NewState)
				{
					Parent->UserEditVariable(VariableName, NewState == ECheckBoxState::Checked);
				});
			InnerPadding.Top = InnerPadding.Bottom = 2;
			break;
		case ESUDSValueType::Gender:
			ValueWidget = SNew(SComboButton)
				.OnGetMenuContent(this, &SSUDSEditorVariableItem::GetGenderMenu)
				.ButtonContent()
				[
					SNew(STextBlock)
						.Text(FText::FromString(VariableValue.ToString()))
						.Font(PropertyFont)
				];
			InnerPadding.Top = InnerPadding.Bottom = 1;

			break;
		case ESUDSValueType::Text:
			ValueWidget = SNew(SEditableTextBox)
				.IsReadOnly(false)
				.Text(VariableValue.GetTextValue())
				.Font(PropertyFont)
				.OnTextCommitted_Lambda([this] (const FText& InValue, ETextCommit::Type)
				{
					Parent->UserEditVariable(VariableName, InValue);
				});
			break;
		case ESUDSValueType::Name:
			ValueWidget = SNew(SEditableTextBox)
				.IsReadOnly(false)
				.Text(FText::FromString(VariableValue.GetNameValue().ToString()))
				.Font(PropertyFont)
				.OnTextCommitted_Lambda([this] (const FText& InValue, ETextCommit::Type)
				{
					Parent->UserEditVariable(VariableName, FSUDSValue(FName(InValue.ToString()), false));
				});
			break;
		case ESUDSValueType::Variable:
		case ESUDSValueType::Empty:
		default:
			ValueWidget = SNew(STextBlock);
			break;
			
		};

		const FMargin BoxMargin(2, 1, 0, 1);
		return SNew(SBox)
		.HeightOverride(26)
		.Padding(BoxMargin)
		[
			SNew(SBorder)
			.BorderImage(BorderBrush)
			.BorderBackgroundColor(bIsManualOverride ? ManualOverrideBgColour : NormalBgColour)
			.VAlign(VAlign_Center)
			[
			
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.Padding(InnerPadding)
				.AutoWidth()
				[
					ValueWidget.ToSharedRef()
				]
				+ SHorizontalBox::Slot()
				.Padding(5,2,5,2)
				.FillWidth(1.0)
				.HAlign(HAlign_Right)
				[
					SNew(SButton)
					.ButtonStyle( FAppStyle::Get(), "SimpleButton" )
					.OnClicked_Lambda([this]
					{
						// Delete click
						Parent->DeleteVariable(VariableName);
						return FReply::Handled();
					} )
					.ContentPadding(0)
					.IsFocusable(false)
					[ 
						SNew( SImage )
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION > 0
						.Image( FAppStyle::GetBrush("Icons.Delete") )
#else
						.Image( FEditorStyle::GetBrush("Icons.Delete") )
#endif
						.ColorAndOpacity( FSlateColor::UseForeground() )
					]
					
				]
			]
		];
		

	}
	
}

TSharedRef<SWidget> SSUDSEditorVariableItem::GetGenderMenu()
{
	FMenuBuilder MenuBuilder(true, NULL);

	// NumEnums() - 1 because the last value is the autogen _MAX value
	for (int i = 0; i < StaticEnum<ETextGender>()->NumEnums() - 1; ++i)
	{
		ETextGender Val = (ETextGender)StaticEnum<ETextGender>()->GetValueByIndex(i);
		const FUIAction Action(FExecuteAction::CreateSP(this, &SSUDSEditorVariableItem::OnGenderSelected, Val));
		MenuBuilder.AddMenuEntry(FText::FromString(StaticEnum<ETextGender>()->GetNameStringByIndex(i)),
		                         FText(),
		                         FSlateIcon(),
		                         Action);
		
	}
	
	return MenuBuilder.MakeWidget();
	
}

void SSUDSEditorVariableItem::OnGenderSelected(ETextGender NewGender)
{
	Parent->UserEditVariable(VariableName, NewGender);
}

FVector2D SSUDSEditorVariableItem::ComputeDesiredSize(float X) const
{
	// ItemHeight(28) at table level is totally not working for some reason, so fudge it here
	FVector2D Desired = SMultiColumnTableRow<TSharedPtr<FString, ESPMode::ThreadSafe>>::ComputeDesiredSize(X);
	Desired.Y = 28;
	return Desired;
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
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION > 0
	const FTextBlockStyle& OrigStyle = FAppStyle::Get().GetWidgetStyle<FTextBlockStyle>(LogNormalStyle);
#else
	const FTextBlockStyle& OrigStyle = FEditorStyle::Get().GetWidgetStyle<FTextBlockStyle>(LogNormalStyle);
#endif

	for (const auto Msg : Messages)
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
	const FString LineNoStr = LineNo > 0 ? FString::Printf(TEXT("L%04d"), LineNo) : "     ";
	const FString ConcatLine = FString::Printf(TEXT("%s[%s] %s %s"), *CatPadding, *CatStr, *LineNoStr, *Message);
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
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION > 0
		.Style(FAppStyle::Get(), "Log.TextBox")
#else
		.Style(FEditorStyle::Get(), "Log.TextBox")
		.TextStyle(FEditorStyle::Get(), "Log.Normal")
#endif
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
