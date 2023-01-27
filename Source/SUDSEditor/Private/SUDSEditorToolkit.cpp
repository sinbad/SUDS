#include "SUDSEditorToolkit.h"

#include "SUDSDialogue.h"
#include "SUDSLibrary.h"
#include "SUDSScript.h"

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
		
		DialogueListView = SNew(SListView<TSharedPtr<FSUDSEditorDialogueRow>>)
				.ItemHeight(24)
				.ListItemsSource(&DialogueRows)
				.OnGenerateRow(this, &FSUDSEditorToolkit::OnGenerateRowForDialogue);
		
		return SNew(SDockTab)
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.FillHeight(1)
			[
				DialogueListView.ToSharedRef()
			]
			+SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Left)
			.Padding(30, 15, 30, 15)
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.Padding(0, 0 ,0 , 5)
				[
					SNew(SButton)
					.HAlign(HAlign_Left)
					.Text(INVTEXT("Choice1"))
				]
				+SVerticalBox::Slot()
				.Padding(0, 0 ,0 , 5)
				[
					SNew(SButton)
					.HAlign(HAlign_Left)
					.Text(INVTEXT("Choice2"))
				]
				+SVerticalBox::Slot()
				.Padding(0, 0 ,0 , 5)
				[
					SNew(SButton)
					.HAlign(HAlign_Left)
					.Text(INVTEXT("Choice3"))
				]
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
			SNew(STextBlock)
			.Text(INVTEXT("Variables here"))
		];
	}))
	.SetDisplayName(INVTEXT("Variables"))
	.SetGroup(WorkspaceMenuCategory.ToSharedRef());


	InTabManager->RegisterTabSpawner("SUDSLogTab", FOnSpawnTab::CreateLambda([=](const FSpawnTabArgs&)
	{
		return SNew(SDockTab)
		[
			SNew(STextBlock)
			.Text(INVTEXT("Trace log here"))
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
	DialogueRows.Empty();
	DialogueListView->RequestListRefresh();
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
}

void FSUDSEditorToolkit::OnDialogueChoice(USUDSDialogue* D, int ChoiceIndex)
{
}

void FSUDSEditorToolkit::OnDialogueEvent(USUDSDialogue* D, FName EventName, const TArray<FSUDSValue>& Args)
{
}

void FSUDSEditorToolkit::OnDialogueFinished(USUDSDialogue* D)
{
}

void FSUDSEditorToolkit::OnDialogueProceeding(USUDSDialogue* D)
{
}

void FSUDSEditorToolkit::OnDialogueStarting(USUDSDialogue* D, FName LabelName)
{
}

void FSUDSEditorToolkit::OnDialogueSpeakerLine(USUDSDialogue* D)
{
	DialogueRows.Add(MakeShareable(new FSUDSEditorDialogueRow(D->GetSpeakerDisplayName(), D->GetText())));
	DialogueListView->RequestListRefresh();
}

void FSUDSEditorToolkit::OnDialogueVariableChanged(USUDSDialogue* D,
	FName VariableName,
	const FSUDSValue& ToValue,
	bool bFromScript)
{
}

void FSUDSEditorToolkit::OnDialogueVariableRequested(USUDSDialogue* D, FName VariableName)
{
}

TSharedRef<ITableRow> FSUDSEditorToolkit::OnGenerateRowForDialogue(
	TSharedPtr<FSUDSEditorDialogueRow> Row,
	const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STableRow<TSharedPtr<FSUDSEditorDialogueRow> >, OwnerTable)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0)
			[
				SNew(STextBlock)
				.Text(Row->SpeakerName)
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0)
			[
				SNew(STextBlock)
				.Text(Row->Line)
			]
		];
		
}
