#include "SUDSDialogue.h"
#include "SUDSLibrary.h"
#include "SUDSScript.h"
#include "SUDSScriptImporter.h"
#include "Internationalization/StringTable.h"
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
		:nestedstart
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
								 "SUDSTest.TestSimpleRunning",
								 EAutomationTestFlags::EditorContext |
								 EAutomationTestFlags::ClientContext |
								 EAutomationTestFlags::ProductFilter)



bool FTestSimpleRunning::RunTest(const FString& Parameters)
{
	FSUDSScriptImporter Importer;
	TestTrue("Import should succeed", Importer.ImportFromBuffer(GetData(SimpleRunnerInput), SimpleRunnerInput.Len(), "SimpleRunnerInput", true));

	auto Script = NewObject<USUDSScript>(GetTransientPackage(), "Test");
	auto StringTable = NewObject<UStringTable>(GetTransientPackage(), "TestStrings");
	Importer.PopulateAsset(Script, StringTable->GetMutableStringTable().Get());

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

	TestTrue("Choice 1", Dlg->Choose(0));
	TestText(this, "Choice 1 Text", Dlg, "NPC", "How rude, bye then");
	// Goes straight to end
	TestFalse("Choice 1 Follow On", Dlg->Continue());
	TestTrue("Should be at end", Dlg->IsEnded());

	// Start again
	Dlg->Restart();
	TestText(this, "First node", Dlg, "Player", "Hello there");
	TestEqual("First node choices", Dlg->GetNumberOfChoices(), 1);
	TestTrue("First node choice text", Dlg->GetChoiceText(0).IsEmpty());
	TestTrue("Continue", Dlg->Continue());
	TestText(this, "Node 2", Dlg, "NPC", "Salutations fellow human");

	TestTrue("Choice 2", Dlg->Choose(1));
	TestText(this, "Choice 2 Text", Dlg, "NPC", "Some nesting");
	TestEqual("Choice 2 nested choices", Dlg->GetNumberOfChoices(), 3);
	TestEqual("Choice 2 nested choice text 0", Dlg->GetChoiceText(0).ToString(), "Actually bye");
	TestEqual("Choice 2 nested choice text 1", Dlg->GetChoiceText(1).ToString(), "A fallthrough choice");
	TestEqual("Choice 2 nested choice text 2", Dlg->GetChoiceText(2).ToString(), "A goto choice");
	
	TestTrue("Nested choice made", Dlg->Choose(0));
	TestText(this, "Nested choice made text", Dlg, "Player", "Gotta go!");
	TestTrue("Nested choice follow On", Dlg->Continue());
	TestText(this, "Nested choice follow on text", Dlg, "NPC", "Bye!");
	TestFalse("Nested choice follow On 2", Dlg->Continue());
	TestTrue("Should be at end", Dlg->IsEnded());

	// Start again, this time from nested choice
	Dlg->Restart(true, "nestedstart");
	TestText(this, "nestedchoice restart Text", Dlg, "NPC", "Some nesting");
	TestTrue("Nested choice made", Dlg->Choose(1));
	TestText(this, "Nested choice 2 Text", Dlg, "NPC", "This should fall through to latterhalf");
	TestTrue("Nested choice 2 follow On", Dlg->Continue());
	// Should have fallen through
	TestText(this, "Fallthrough Text", Dlg, "Player", "This is the latter half of the discussion");
	TestTrue("Continue", Dlg->Continue());
	TestText(this, "Fallthrough Text 2", Dlg, "NPC", "Yep, sure is");
	TestEqual("Fallthrough choices", Dlg->GetNumberOfChoices(), 3);
	TestEqual("Fallthrough choice text 0", Dlg->GetChoiceText(0).ToString(), "Go back to choice");
	TestEqual("Fallthrough choice text 1", Dlg->GetChoiceText(1).ToString(), "Return to the start");
	TestEqual("Fallthrough choice text 2", Dlg->GetChoiceText(2).ToString(), "Continue");

	// Go back to choice
	TestTrue("Fallthrough choice made", Dlg->Choose(0));
	TestText(this, "Fallthrough Choice Text", Dlg, "NPC", "Okay!");
	// The Goto choice should have collapsed the choices such that we can get them immediately
	TestEqual("Fallthrough then goto choices", Dlg->GetNumberOfChoices(), 3);
	TestEqual("Fallthrough then goto choice text 0", Dlg->GetChoiceText(0).ToString(), "Actually no");
	TestEqual("Fallthrough then goto choice text 1", Dlg->GetChoiceText(1).ToString(), "Nested option");
	TestEqual("Fallthrough then goto choice text 2", Dlg->GetChoiceText(2).ToString(), "Another option");

	// Restart to test another path
	Dlg->Restart(true, "nestedstart");
	TestText(this, "nestedchoice restart Text", Dlg, "NPC", "Some nesting");
	TestTrue("Nested choice made", Dlg->Choose(2));
	// This should be a direct goto to latterhalf
	TestText(this, "Direct goto", Dlg, "Player", "This is the latter half of the discussion");
	
	
	Dlg->Restart(true);
	TestTrue("Continue", Dlg->Continue());
	TestTrue("Choice 3", Dlg->Choose(2));
	TestText(this, "Choice 3 Text", Dlg, "Player", "What now?");
	TestTrue("Continue", Dlg->Continue());
	TestText(this, "Choice 3 Text 2", Dlg, "NPC", "This is another fallthrough");
	TestTrue("Continue", Dlg->Continue());
	// Should have fallen through
	TestText(this, "Direct goto", Dlg, "Player", "This is the latter half of the discussion");
	
	return true;
}


PRAGMA_ENABLE_OPTIMIZATION