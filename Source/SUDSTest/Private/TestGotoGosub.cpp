#include "SUDSDialogue.h"
#include "SUDSLibrary.h"
#include "SUDSMessageLogger.h"
#include "SUDSScript.h"
#include "SUDSScriptImporter.h"
#include "TestUtils.h"
#include "Misc/AutomationTest.h"

UE_DISABLE_OPTIMIZATION

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
	* Gosub choice
		NPC: This is going to return to the choice
		[gosub subroutine]
		* Choice After
			NPC: It's a choice after a gosub!
			[goto goodbye]
		* Choice After 2
			NPC: Not really much difference eh
			[goto goodbye]
	* Gosub choice via goto
		NPC: This is going to return to the choice via goto
		[gosub subroutine]
		[goto choice]

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
	FSUDSMessageLogger Logger(false);
	FSUDSScriptImporter Importer;
	TestTrue("Import should succeed", Importer.ImportFromBuffer(GetData(GotoGosubInput), GotoGosubInput.Len(), "GotoGosubInput", &Logger, true));

	auto Script = NewObject<USUDSScript>(GetTransientPackage(), "Test");
	const ScopedStringTableHolder StringTableHolder;
	Importer.PopulateAsset(Script, StringTableHolder.StringTable);

	// Script shouldn't be the owner of the dialogue but it's the only UObject we've got right now so why not
	auto Dlg = USUDSLibrary::CreateDialogue(Script, Script);
	Dlg->Start();

	
	TestDialogueText(this, "Start node", Dlg, "Player", "Hello there");
	TestTrue("Continue", Dlg->Continue());
	TestDialogueText(this, "Next", Dlg, "NPC", "Hello");
	TestEqual("Choices", Dlg->GetNumberOfChoices(), 4);
	TestEqual("Choice 0 text", Dlg->GetChoiceText(0).ToString(), "Actually bye");
	TestTrue("Choose 0", Dlg->Choose(0));
	TestDialogueText(this, "Next", Dlg, "NPC", "How rude");
	TestFalse("End", Dlg->Continue());

	Dlg->Restart();
	TestDialogueText(this, "Start node", Dlg, "Player", "Hello there");
	TestTrue("Continue", Dlg->Continue());
	TestDialogueText(this, "Next", Dlg, "NPC", "Hello");
	TestTrue("Choose 1", Dlg->Choose(1));
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

	Dlg->Restart();
	TestDialogueText(this, "Start node", Dlg, "Player", "Hello there");
	TestTrue("Continue", Dlg->Continue());
	TestDialogueText(this, "Next", Dlg, "NPC", "Hello");
	TestTrue("Choose 2", Dlg->Choose(2));
	TestDialogueText(this, "Next", Dlg, "NPC", "This is going to return to the choice");
	TestTrue("Continue", Dlg->Continue());
	TestDialogueText(this, "Next", Dlg, "Player", "Some reused discussion");
	TestTrue("Continue", Dlg->Continue());
	TestDialogueText(this, "Next", Dlg, "NPC", "Yep, sure is");
	// Should have a choice at the end of it this time
	TestEqual("Choices", Dlg->GetNumberOfChoices(), 2);
	TestEqual("Choice 0 text", Dlg->GetChoiceText(0).ToString(), "Choice After");
	TestEqual("Choice 1 text", Dlg->GetChoiceText(1).ToString(), "Choice After 2");
	TestTrue("Choose 1", Dlg->Choose(1));
	TestDialogueText(this, "Next", Dlg, "NPC", "Not really much difference eh");
	TestTrue("Continue", Dlg->Continue());
	TestDialogueText(this, "Next", Dlg, "NPC", "Bye!");
	TestFalse("Continue", Dlg->Continue());

	Dlg->Restart();
	TestDialogueText(this, "Start node", Dlg, "Player", "Hello there");
	TestTrue("Continue", Dlg->Continue());
	TestDialogueText(this, "Next", Dlg, "NPC", "Hello");
	TestTrue("Choose 2", Dlg->Choose(3));
	TestDialogueText(this, "Next", Dlg, "NPC", "This is going to return to the choice via goto");
	TestTrue("Continue", Dlg->Continue());
	TestDialogueText(this, "Next", Dlg, "Player", "Some reused discussion");
	TestTrue("Continue", Dlg->Continue());
	TestDialogueText(this, "Next", Dlg, "NPC", "Yep, sure is");
	// Should have a choice at the end of it this time
	TestEqual("Choices", Dlg->GetNumberOfChoices(), 4);
	TestEqual("Choice 0 text", Dlg->GetChoiceText(0).ToString(), "Actually bye");
	TestTrue("Choose 0", Dlg->Choose(0));
	TestDialogueText(this, "Next", Dlg, "NPC", "How rude");
	TestFalse("End", Dlg->Continue());

	Script->MarkAsGarbage();
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
	FSUDSMessageLogger Logger(false);
	FSUDSScriptImporter Importer;
	TestTrue("Import should succeed", Importer.ImportFromBuffer(GetData(NestedGosubInput), NestedGosubInput.Len(), "NestedGosubInput", &Logger, true));

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

	Script->MarkAsGarbage();
	return true;
}



const FString GotoBetweenSpeakerAndChoiceInput = R"RAWSUD(
Player: Hello there
[if {SkipChoices}]
	[goto end]
[endif]
* Option A
* Option B
)RAWSUD";

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestGotoBetweenSpeakerAndChoice1,
	"SUDSTest.TestGotoBetweenSpeakerAndChoice1",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::ClientContext |
	EAutomationTestFlags::ProductFilter)



bool FTestGotoBetweenSpeakerAndChoice1::RunTest(const FString& Parameters)
{
	FSUDSMessageLogger Logger(false);
	FSUDSScriptImporter Importer;
	TestTrue("Import should succeed", Importer.ImportFromBuffer(GetData(GotoBetweenSpeakerAndChoiceInput), GotoBetweenSpeakerAndChoiceInput.Len(), "GotoBetweenSpeakerAndChoiceInput", &Logger, true));

	auto Script = NewObject<USUDSScript>(GetTransientPackage(), "Test");
	const ScopedStringTableHolder StringTableHolder;
	Importer.PopulateAsset(Script, StringTableHolder.StringTable);

	// Script shouldn't be the owner of the dialogue but it's the only UObject we've got right now so why not
	auto Dlg = USUDSLibrary::CreateDialogue(Script, Script);
	Dlg->Start();


	if (TestEqual("Choice Count", Dlg->GetNumberOfChoices(), 2))
	{
		TestEqual("Choice 1", Dlg->GetChoiceText(0).ToString(), "Option A");
		TestEqual("Choice 2", Dlg->GetChoiceText(1).ToString(), "Option B");
	}

	Script->MarkAsGarbage();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestGotoBetweenSpeakerAndChoice2,
	"SUDSTest.TestGotoBetweenSpeakerAndChoice2",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::ClientContext |
	EAutomationTestFlags::ProductFilter)



bool FTestGotoBetweenSpeakerAndChoice2::RunTest(const FString& Parameters)
{
	FSUDSMessageLogger Logger(false);
	FSUDSScriptImporter Importer;
	TestTrue("Import should succeed", Importer.ImportFromBuffer(GetData(GotoBetweenSpeakerAndChoiceInput), GotoBetweenSpeakerAndChoiceInput.Len(), "GotoBetweenSpeakerAndChoiceInput", &Logger, true));

	auto Script = NewObject<USUDSScript>(GetTransientPackage(), "Test");
	const ScopedStringTableHolder StringTableHolder;
	Importer.PopulateAsset(Script, StringTableHolder.StringTable);

	// Script shouldn't be the owner of the dialogue but it's the only UObject we've got right now so why not
	auto Dlg = USUDSLibrary::CreateDialogue(Script, Script);
	Dlg->SetVariableBoolean("SkipChoices", true);
	Dlg->Start();


	TestDialogueText(this, "Start node", Dlg, "Player", "Hello there");
	TestTrue("Plain continue", Dlg->IsSimpleContinue());

	Script->MarkAsGarbage();
	return true;
}


const FString GosubBetweenSpeakerAndChoiceInput1 = R"RAWSUD(
Player: Hello there
[gosub MaybeSkipChoices]
* Option A
* Option B
[goto end]

:MaybeSkipChoices
[if {SkipChoices}]
    [goto end]
[endif]
[return]
)RAWSUD";

const FString GosubBetweenSpeakerAndChoiceInput2 = R"RAWSUD(
Player: Hello there
[gosub PrintDebug]
[gosub MaybeSkipChoices]
* Option A
* Option B
[goto end]

:PrintDebug
Debug: SkipChoices is {SkipChoices}
[return]

:MaybeSkipChoices
[if {SkipChoices}]
    [goto end]
[endif]
Speaker: Another sentence.
[return]
)RAWSUD";

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestGosubBetweenSpeakerAndChoice1,
								 "SUDSTest.TestGosubBetweenSpeakerAndChoice1",
								 EAutomationTestFlags::EditorContext |
								 EAutomationTestFlags::ClientContext |
								 EAutomationTestFlags::ProductFilter)



bool FTestGosubBetweenSpeakerAndChoice1::RunTest(const FString& Parameters)
{
	FSUDSMessageLogger Logger(false);
	FSUDSScriptImporter Importer;
	TestTrue("Import should succeed", Importer.ImportFromBuffer(GetData(GosubBetweenSpeakerAndChoiceInput1), GosubBetweenSpeakerAndChoiceInput1.Len(), "GosubBetweenSpeakerAndChoiceInput1", &Logger, true));

	auto Script = NewObject<USUDSScript>(GetTransientPackage(), "Test");
	const ScopedStringTableHolder StringTableHolder;
	Importer.PopulateAsset(Script, StringTableHolder.StringTable);

	// Script shouldn't be the owner of the dialogue but it's the only UObject we've got right now so why not
	auto Dlg = USUDSLibrary::CreateDialogue(Script, Script);
	Dlg->Start();

	
	TestDialogueText(this, "Start node", Dlg, "Player", "Hello there");
	if (TestEqual("Choice Count", Dlg->GetNumberOfChoices(), 2))
	{
		TestEqual("Choice 1", Dlg->GetChoiceText(0).ToString(), "Option A");
		TestEqual("Choice 2", Dlg->GetChoiceText(1).ToString(), "Option B");
	}

	// Now test the true case
	Dlg->SetVariableBoolean("SkipChoices", true);
	Dlg->Restart(false);
	TestDialogueText(this, "Start node", Dlg, "Player", "Hello there");
	TestTrue("Plain continue", Dlg->IsSimpleContinue());	

	Script->MarkAsGarbage();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestGosubBetweenSpeakerAndChoice2,
								 "SUDSTest.TestGosubBetweenSpeakerAndChoice2",
								 EAutomationTestFlags::EditorContext |
								 EAutomationTestFlags::ClientContext |
								 EAutomationTestFlags::ProductFilter)



bool FTestGosubBetweenSpeakerAndChoice2::RunTest(const FString& Parameters)
{
	FSUDSMessageLogger Logger(false);
	FSUDSScriptImporter Importer;
	TestTrue("Import should succeed", Importer.ImportFromBuffer(GetData(GosubBetweenSpeakerAndChoiceInput2), GosubBetweenSpeakerAndChoiceInput2.Len(), "GosubBetweenSpeakerAndChoiceInput2", &Logger, true));

	auto Script = NewObject<USUDSScript>(GetTransientPackage(), "Test");
	const ScopedStringTableHolder StringTableHolder;
	Importer.PopulateAsset(Script, StringTableHolder.StringTable);

	// Script shouldn't be the owner of the dialogue but it's the only UObject we've got right now so why not
	auto Dlg = USUDSLibrary::CreateDialogue(Script, Script);
	Dlg->Start();

	
	TestDialogueText(this, "Start node", Dlg, "Player", "Hello there");
	// in this case, there's another speaker line in both nested gosubs so the choices are actually associated with the
	// last of those.
	TestTrue("Plain continue", Dlg->Continue());
	TestDialogueText(this, "Gosub 1 speaker node", Dlg, "Debug", "SkipChoices is {SkipChoices}");
	TestTrue("Plain continue", Dlg->Continue());
	TestDialogueText(this, "Gosub 2 speaker node", Dlg, "Speaker", "Another sentence.");
	
	if (TestEqual("Choice Count", Dlg->GetNumberOfChoices(), 2))
	{
		TestEqual("Choice 1", Dlg->GetChoiceText(0).ToString(), "Option A");
		TestEqual("Choice 2", Dlg->GetChoiceText(1).ToString(), "Option B");
	}

	// Now test the true case
	Dlg->SetVariableBoolean("SkipChoices", true);
	Dlg->Restart(false);
	TestDialogueText(this, "Start node", Dlg, "Player", "Hello there");
	TestTrue("Plain continue", Dlg->Continue());
	TestDialogueText(this, "Gosub 1 speaker node", Dlg, "Debug", "SkipChoices is 1");
	TestTrue("Plain continue", Dlg->IsSimpleContinue());	
	

	Script->MarkAsGarbage();
	return true;
}


UE_ENABLE_OPTIMIZATION