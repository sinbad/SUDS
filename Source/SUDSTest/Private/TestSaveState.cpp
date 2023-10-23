#include "SUDSDialogue.h"
#include "SUDSLibrary.h"
#include "SUDSMessageLogger.h"
#include "SUDSScript.h"
#include "SUDSScriptImporter.h"
#include "TestUtils.h"
#include "Misc/AutomationTest.h"

UE_DISABLE_OPTIMIZATION

const FString SaveStateInput = R"RAWSUD(
===
# To confirm that when resuming we don't run headers again
[set y = -2.4]
===
[if {alreadyvisited}]
    [goto secondvisit]
[endif]

NPC: Hello
    * First choice
        Player: I took the 1.1 choice
[if {x} > 0]
    * Second choice (conditional)
        Player: I took the 1.2 choice
    * Third choice (conditional)
        Player: I took the 1.3 choice
[else]
    * Second Alt Choice
        Player: I took the alt 1.2 choice        
[endif]
    * Common last choice
        Player: I took the 1.4 choice

[goto goodbye]

:secondvisit

NPC: Hello again you!

[if {y} < 0]
    Player: Y is less than 0
    NPC: How interesting
        * You don't sound that interested
            NPC: Well I was trying to be nice
        [if {x} == 0]
            * Also x is zero
                NPC: Fascinating
        [endif]
        * Well, better be off
            [goto goodbye]
[elseif {y} == 0]
    Player: Y is zero
    NPC: So is my interest level
    Player: Rude
    
[else]
    Player: Who knows what Y is anyway
    [if {ponderous}]
        * Who knows what anything is?
            NPC: Get out
    [endif]
    [if {y} > 4.99]
        * It's more than I can count on one hand
            NPC: Well, that helps
    [else]
        * It's kind of small though
            NPC: Much like your brain
    [endif]
        * I'm done with this
[endif]

Player: This is some fallthrough text

:goodbye
NPC: Bye
)RAWSUD";


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestSaveState,
								 "SUDSTest.TestSaveState",
								 EAutomationTestFlags::EditorContext |
								 EAutomationTestFlags::ClientContext |
								 EAutomationTestFlags::ProductFilter)



bool FTestSaveState::RunTest(const FString& Parameters)
{
	FSUDSMessageLogger Logger(false);
	FSUDSScriptImporter Importer;
	TestTrue("Import should succeed", Importer.ImportFromBuffer(GetData(SaveStateInput), SaveStateInput.Len(), "SaveStateInput", &Logger, true));

	auto Script = NewObject<USUDSScript>(GetTransientPackage(), "Test");
	const ScopedStringTableHolder StringTableHolder;
	Importer.PopulateAsset(Script, StringTableHolder.StringTable);

	// Script shouldn't be the owner of the dialogue but it's the only UObject we've got right now so why not
	auto Dlg = USUDSLibrary::CreateDialogue(Script, Script);
    // Set x before start
    // No point settings y because that gets changed by headers
    Dlg->SetVariableInt("x", 5);
	Dlg->Start();

    TestDialogueText(this, "Text Node", Dlg, "NPC", "Hello");
    if (!TestEqual("Num choices", Dlg->GetNumberOfChoices(), 4))
        return true;
    
    TestEqual("ChoiceText", Dlg->GetChoiceText(0).ToString(), "First choice");
    TestEqual("ChoiceText", Dlg->GetChoiceText(1).ToString(), "Second choice (conditional)");
    TestEqual("ChoiceText", Dlg->GetChoiceText(2).ToString(), "Third choice (conditional)");
    TestEqual("ChoiceText", Dlg->GetChoiceText(3).ToString(), "Common last choice");
    TestTrue("Choose", Dlg->Choose(2));
    TestDialogueText(this, "Text node", Dlg, "Player", "I took the 1.3 choice");

    // Set value of y to test it's retained, and not reset by running headers
    Dlg->SetVariableFloat("y", 23.5f);

    // Save it here
    auto SaveState = Dlg->GetSavedState();

    // Re-construct the dialogue & restore
    auto Dlg2 = USUDSLibrary::CreateDialogue(Script, Script);
    Dlg2->RestoreSavedState(SaveState);

    // We should be back at the same point
    TestDialogueText(this, "Text node", Dlg2, "Player", "I took the 1.3 choice");
    // Check vars
    TestEqual("x value", Dlg2->GetVariableInt("x"), 5);
    TestEqual("y value", Dlg2->GetVariableFloat("y"), 23.5f);
    TestTrue("Continue", Dlg2->Continue());
    TestDialogueText(this, "Text node", Dlg2, "NPC", "Bye");

    // Restart to check choices were remembered
    Dlg2->Restart();
    TestDialogueText(this, "Text Node", Dlg2, "NPC", "Hello");
    TestFalse("Choice not taken", Dlg2->HasChoiceIndexBeenTakenPreviously(0));
    TestFalse("Choice not taken", Dlg2->HasChoiceIndexBeenTakenPreviously(1));
    TestTrue("Choice not taken", Dlg2->HasChoiceIndexBeenTakenPreviously(2));
    TestFalse("Choice not taken", Dlg2->HasChoiceIndexBeenTakenPreviously(3));

    Script->MarkAsGarbage();
	return true;
}

UE_ENABLE_OPTIMIZATION