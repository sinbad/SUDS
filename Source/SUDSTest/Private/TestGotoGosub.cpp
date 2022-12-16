#include "SUDSDialogue.h"
#include "SUDSLibrary.h"
#include "SUDSScript.h"
#include "SUDSScriptImporter.h"
#include "TestUtils.h"
#include "Misc/AutomationTest.h"

PRAGMA_DISABLE_OPTIMIZATION

const FString GotoGosubInput = R"RAWSUD(
Player: Hello there
NPC: Hello
	:choice
	* Actually bye
		NPC: How rude
		[goto end]
	* Reused
		NPC: This is going to re-used dialogue
		[gosub subroutine]
		NPC: And now we're back
		[goto goodbye]

:subroutine
Player: Some reused discussion
NPC: Yep, sure is
[return]
:goodbye
NPC: Bye!
)RAWSUD";

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestGotoGosub,
								 "SUDSTest.TestGotoGosub",
								 EAutomationTestFlags::EditorContext |
								 EAutomationTestFlags::ClientContext |
								 EAutomationTestFlags::ProductFilter)



bool FTestGotoGosub::RunTest(const FString& Parameters)
{
	FSUDSScriptImporter Importer;
	TestTrue("Import should succeed", Importer.ImportFromBuffer(GetData(GotoGosubInput), GotoGosubInput.Len(), "GotoGosubInput", true));

	auto Script = NewObject<USUDSScript>(GetTransientPackage(), "Test");
	const ScopedStringTableHolder StringTableHolder;
	Importer.PopulateAsset(Script, StringTableHolder.StringTable);

	// Script shouldn't be the owner of the dialogue but it's the only UObject we've got right now so why not
	auto Dlg = USUDSLibrary::CreateDialogue(Script, Script);
	Dlg->Start();

	
	TestDialogueText(this, "Start node", Dlg, "Player", "Hello there");
	TestTrue("Continue", Dlg->Continue());
	TestDialogueText(this, "Next", Dlg, "NPC", "Hello");
	TestEqual("Choices", Dlg->GetNumberOfChoices(), 2);
	TestEqual("Choice 0 text", Dlg->GetChoiceText(0).ToString(), "Actually bye");
	TestTrue("Choose 0", Dlg->Choose(0));
	TestDialogueText(this, "Next", Dlg, "NPC", "How rude");
	TestFalse("End", Dlg->Continue());

	Dlg->Restart();
	TestDialogueText(this, "Start node", Dlg, "Player", "Hello there");
	TestTrue("Continue", Dlg->Continue());
	TestDialogueText(this, "Next", Dlg, "NPC", "Hello");
	TestTrue("Choose 0", Dlg->Choose(1));
	TestDialogueText(this, "Next", Dlg, "NPC", "This is going to re-used dialogue");
	TestTrue("Continue", Dlg->Continue());
	TestDialogueText(this, "Next", Dlg, "Player", "Some reused discussion");
	TestTrue("Continue", Dlg->Continue());
	TestDialogueText(this, "Next", Dlg, "NPC", "Yep, sure is");
	TestTrue("Continue", Dlg->Continue());
	TestDialogueText(this, "Next", Dlg, "NPC", "And now we're back");
	TestTrue("Continue", Dlg->Continue());
	TestDialogueText(this, "Next", Dlg, "NPC", "Bye!");
	TestFalse("Continue", Dlg->Continue());

	return true;
}


const FString NestedGosubInput = R"RAWSUD(
Player: Hello there
[gosub sub1]
NPC: Back at level 0
[goto goodbye]

:sub1
Player: This is level 1
[gosub sub2_1]
[gosub sub2_2]
Player: End of level 1
[return]

:sub2_1
Player: This is level 2, sub 1
[return]


:sub2_2
Player: This is level 2, sub 2
NPC: We have to go deeper
[gosub sub3]
Player: End of level 2, sub 2
[return]

:sub3
Player: This is level 3
[return]

:goodbye
NPC: Bye!
)RAWSUD";

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestNestedGosub,
								 "SUDSTest.TestNestedGosub",
								 EAutomationTestFlags::EditorContext |
								 EAutomationTestFlags::ClientContext |
								 EAutomationTestFlags::ProductFilter)



bool FTestNestedGosub::RunTest(const FString& Parameters)
{
	FSUDSScriptImporter Importer;
	TestTrue("Import should succeed", Importer.ImportFromBuffer(GetData(NestedGosubInput), NestedGosubInput.Len(), "NestedGosubInput", true));

	auto Script = NewObject<USUDSScript>(GetTransientPackage(), "Test");
	const ScopedStringTableHolder StringTableHolder;
	Importer.PopulateAsset(Script, StringTableHolder.StringTable);

	// Script shouldn't be the owner of the dialogue but it's the only UObject we've got right now so why not
	auto Dlg = USUDSLibrary::CreateDialogue(Script, Script);
	Dlg->Start();

	
	TestDialogueText(this, "Start node", Dlg, "Player", "Hello there");
	TestTrue("Continue", Dlg->Continue());
	TestDialogueText(this, "Sub", Dlg, "Player", "This is level 1");
	TestTrue("Continue", Dlg->Continue());
	TestDialogueText(this, "Sub", Dlg, "Player", "This is level 2, sub 1");
	TestTrue("Continue", Dlg->Continue());
	TestDialogueText(this, "Sub", Dlg, "Player", "This is level 2, sub 2");
	TestTrue("Continue", Dlg->Continue());
	TestDialogueText(this, "Sub", Dlg, "NPC", "We have to go deeper");
	TestTrue("Continue", Dlg->Continue());
	TestDialogueText(this, "Sub", Dlg, "Player", "This is level 3");
	TestTrue("Continue", Dlg->Continue());
	TestDialogueText(this, "Sub", Dlg, "Player", "End of level 2, sub 2");
	TestTrue("Continue", Dlg->Continue());
	TestDialogueText(this, "Sub", Dlg, "Player", "End of level 1");
	TestTrue("Continue", Dlg->Continue());
	TestDialogueText(this, "Sub", Dlg, "NPC", "Back at level 0");
	TestTrue("Continue", Dlg->Continue());
	TestDialogueText(this, "Bye", Dlg, "NPC", "Bye!");
	TestFalse("Continue", Dlg->Continue());

	return true;
}


PRAGMA_ENABLE_OPTIMIZATION