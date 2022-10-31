#include "SUDSDialogue.h"
#include "SUDSLibrary.h"
#include "SUDSScript.h"
#include "SUDSScriptImporter.h"
#include "Misc/AutomationTest.h"

PRAGMA_DISABLE_OPTIMIZATION

const FString SimpleRunnerInput = R"RAWSUD(
:start
Player: Hello there
NPC: Salutations fellow human
	:choice
	* Actually no
		NPC: How rude, bye then
		[goto end]
	* Nested option
		NPC: Some nesting
		* Actually bye
			Player: Gotta go!
			[go to goodbye] 
		* A fallthrough choice
			NPC: This should fall through to latterhalf
		* A goto choice
			[goto latterhalf]
	* Another option
		Player: What now?
		NPC: This is another fallthrough
:latterhalf
Player: This is the latter half of the discussion
NPC: Yep, sure is
	* Go back to choice
		NPC: Okay!
		[goto choice]
	* Return to the start
		NPC: Gotcha
		[goto start]
	* Continue
		Player: OK I'd like to carry on now 
		NPC: Right you are guv, falling through
:goodbye
NPC: Bye!
)RAWSUD";


void TestText(FAutomationTestBase* T, const FString& NameForTest, USUDSDialogue* D, const FString& SpeakerID, const FString& Text)
{
	T->TestEqual(NameForTest, D->GetCurrentSpeakerID(), SpeakerID);
	T->TestEqual(NameForTest, D->GetCurrentText().ToString(), Text);
	
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestSimpleRunning,
								 "SUDSTest.TestConversionToRuntime",
								 EAutomationTestFlags::EditorContext |
								 EAutomationTestFlags::ClientContext |
								 EAutomationTestFlags::ProductFilter)



bool FTestSimpleRunning::RunTest(const FString& Parameters)
{
	FSUDSScriptImporter Importer;
	TestTrue("Import should succeed", Importer.ImportFromBuffer(GetData(SimpleRunnerInput), SimpleRunnerInput.Len(), "SimpleRunnerInput", true));

	auto Script = NewObject<USUDSScript>(GetTransientPackage(), "Test");
	Importer.PopulateAsset(Script);

	// Script shouldn't be the owner of the dialogue but it's the only UObject we've got right now so why not
	auto Dlg = USUDSLibrary::CreateDialogue(Script, Script);

	TestText(this, "First node", Dlg, "Player", "Hello there");
	TestEqual("First node choices", Dlg->GetNumberOfChoices(), 1);
	TestTrue("First node choice text", Dlg->GetChoiceText(0).IsEmpty());

	TestTrue("Continue", Dlg->Continue());

	TestText(this, "Node 2", Dlg, "NPC", "Salutations fellow human");
	TestEqual("Node 2 choices", Dlg->GetNumberOfChoices(), 3);
	TestEqual("Node 2 choice text 0", Dlg->GetChoiceText(0).ToString(), "Actually no");
	TestEqual("Node 2 choice text 1", Dlg->GetChoiceText(1).ToString(), "Nested option");
	TestEqual("Node 2 choice text 2", Dlg->GetChoiceText(2).ToString(), "Another option");


	return true;
}


PRAGMA_ENABLE_OPTIMIZATION