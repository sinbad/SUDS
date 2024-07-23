#include "SUDSLibrary.h"
#include "SUDSMessageLogger.h"
#include "SUDSScript.h"
#include "SUDSScriptImporter.h"
#include "TestUtils.h"

UE_DISABLE_OPTIMIZATION

const FString BasicRandomInput = R"RAWSUD(
Player: Hello
:start
[random]
    NPC: Reply when random == 0
[or]
    NPC: Reply when random == 1
[or]
    NPC: Reply when random == 2
[endrandom]
Player: OK
[goto start]
)RAWSUD";


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestRandomBasics,
								 "SUDSTest.TestRandomBasics",
								 EAutomationTestFlags::EditorContext |
								 EAutomationTestFlags::ClientContext |
								 EAutomationTestFlags::ProductFilter)


bool FTestRandomBasics::RunTest(const FString& Parameters)
{
    FSUDSMessageLogger Logger(false);
    FSUDSScriptImporter Importer;
    TestTrue("Import should succeed", Importer.ImportFromBuffer(GetData(BasicRandomInput), BasicRandomInput.Len(), "BasicRandomInput", &Logger, true));

    auto Script = NewObject<USUDSScript>(GetTransientPackage(), "Test");
    const ScopedStringTableHolder StringTableHolder;
    Importer.PopulateAsset(Script, StringTableHolder.StringTable);

    // Script shouldn't be the owner of the dialogue but it's the only UObject we've got right now so why not
    auto Dlg = USUDSLibrary::CreateDialogue(Script, Script);

    // Seed random so we have consistent results
    FMath::SRandInit(34);
    Dlg->Start();

    TestDialogueText(this, "Text node", Dlg, "Player", "Hello");
    TestTrue("Continue", Dlg->Continue());
    TestDialogueText(this, "Random node", Dlg, "NPC", "Reply when random == 2");
    TestTrue("Continue", Dlg->Continue());
    TestDialogueText(this, "Final node", Dlg, "Player", "OK");

    // Should loop, run random again 
    TestTrue("Continue", Dlg->Continue());
    TestDialogueText(this, "Random node", Dlg, "NPC", "Reply when random == 1");
    TestTrue("Continue", Dlg->Continue());
    TestDialogueText(this, "Final node", Dlg, "Player", "OK");

    // Should loop, run random again 
    TestTrue("Continue", Dlg->Continue());
    TestDialogueText(this, "Random node", Dlg, "NPC", "Reply when random == 0");
    TestTrue("Continue", Dlg->Continue());
    TestDialogueText(this, "Final node", Dlg, "Player", "OK");

    // Should loop, run random again 
    TestTrue("Continue", Dlg->Continue());
    TestDialogueText(this, "Random node", Dlg, "NPC", "Reply when random == 1");
    TestTrue("Continue", Dlg->Continue());
    TestDialogueText(this, "Final node", Dlg, "Player", "OK");
    
    Script->MarkAsGarbage();
    return true;
    
}

const FString NestedRandomInput = R"RAWSUD(
Player: Hello
:start
[random]
    NPC: Reply when random == 0
[or]
    [random]
        NPC: Reply when random == 1 && subrandom == 0
    [or]
        NPC: Reply when random == 1 && subrandom == 1
    [endrandom]
[or]
    NPC: Reply when random == 2
[endrandom]
Player: OK
[goto start]
)RAWSUD";


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestRandomNested,
								 "SUDSTest.TestRandomNested",
								 EAutomationTestFlags::EditorContext |
								 EAutomationTestFlags::ClientContext |
								 EAutomationTestFlags::ProductFilter)


bool FTestRandomNested::RunTest(const FString& Parameters)
{
    FSUDSMessageLogger Logger(false);
    FSUDSScriptImporter Importer;
    TestTrue("Import should succeed", Importer.ImportFromBuffer(GetData(NestedRandomInput), NestedRandomInput.Len(), "NestedRandomInput", &Logger, true));

    auto Script = NewObject<USUDSScript>(GetTransientPackage(), "Test");
    const ScopedStringTableHolder StringTableHolder;
    Importer.PopulateAsset(Script, StringTableHolder.StringTable);

    // Script shouldn't be the owner of the dialogue but it's the only UObject we've got right now so why not
    auto Dlg = USUDSLibrary::CreateDialogue(Script, Script);

    // Seed random so we have consistent results
    FMath::SRandInit(785);
    Dlg->Start();

    TestDialogueText(this, "Text node", Dlg, "Player", "Hello");
    TestTrue("Continue", Dlg->Continue());

    TestDialogueText(this, "Random node", Dlg, "NPC", "Reply when random == 0");
    TestTrue("Continue", Dlg->Continue());
    TestDialogueText(this, "Final node", Dlg, "Player", "OK");
    TestTrue("Continue", Dlg->Continue());

    // Restart
    TestDialogueText(this, "Random node", Dlg, "NPC", "Reply when random == 1 && subrandom == 1");
    TestTrue("Continue", Dlg->Continue());
    TestDialogueText(this, "Final node", Dlg, "Player", "OK");
    TestTrue("Continue", Dlg->Continue());
    
    // Restart
    TestDialogueText(this, "Random node", Dlg, "NPC", "Reply when random == 1 && subrandom == 0");
    TestTrue("Continue", Dlg->Continue());
    TestDialogueText(this, "Final node", Dlg, "Player", "OK");
    TestTrue("Continue", Dlg->Continue());

    // Restart
    TestDialogueText(this, "Random node", Dlg, "NPC", "Reply when random == 2");
    TestTrue("Continue", Dlg->Continue());
    TestDialogueText(this, "Final node", Dlg, "Player", "OK");
    TestTrue("Continue", Dlg->Continue());

    // Restart
    TestDialogueText(this, "Random node", Dlg, "NPC", "Reply when random == 0");
    TestTrue("Continue", Dlg->Continue());
    TestDialogueText(this, "Final node", Dlg, "Player", "OK");
    TestTrue("Continue", Dlg->Continue());
    
    Script->MarkAsGarbage();
    return true;
    
}

const FString SiblingRandomInput = R"RAWSUD(
Player: Hello
:start
[random]
    NPC: Reply when random == 0
[or]
    NPC: Reply when random == 1
[or]
    NPC: Reply when random == 2
[endrandom]
[random]
    NPC: Second random == 0
[or]
    NPC: Second random == 1
[or]
    NPC: Second random == 2
[endrandom]
Player: OK
[goto start]
)RAWSUD";

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestRandomSibling,
                                 "SUDSTest.TestRandomSibling",
                                 EAutomationTestFlags::EditorContext |
                                 EAutomationTestFlags::ClientContext |
                                 EAutomationTestFlags::ProductFilter)
bool FTestRandomSibling::RunTest(const FString& Parameters)
{
    FSUDSMessageLogger Logger(false);
    FSUDSScriptImporter Importer;
    TestTrue("Import should succeed", Importer.ImportFromBuffer(GetData(SiblingRandomInput), SiblingRandomInput.Len(), "SiblingRandomInput", &Logger, true));

    auto Script = NewObject<USUDSScript>(GetTransientPackage(), "Test");
    const ScopedStringTableHolder StringTableHolder;
    Importer.PopulateAsset(Script, StringTableHolder.StringTable);

    // Script shouldn't be the owner of the dialogue but it's the only UObject we've got right now so why not
    auto Dlg = USUDSLibrary::CreateDialogue(Script, Script);

    // Seed random so we have consistent results
    FMath::SRandInit(2376);
    Dlg->Start();

    TestDialogueText(this, "Text node", Dlg, "Player", "Hello");
    TestTrue("Continue", Dlg->Continue());


    TestDialogueText(this, "Text node", Dlg, "NPC", "Reply when random == 1");
    TestTrue("Continue", Dlg->Continue());
    TestDialogueText(this, "Text node", Dlg, "NPC", "Second random == 0");
    TestTrue("Continue", Dlg->Continue());
    TestDialogueText(this, "Text node", Dlg, "Player", "OK");
    TestTrue("Continue", Dlg->Continue());

    // Restart
    TestDialogueText(this, "Text node", Dlg, "NPC", "Reply when random == 0");
    TestTrue("Continue", Dlg->Continue());
    TestDialogueText(this, "Text node", Dlg, "NPC", "Second random == 2");
    TestTrue("Continue", Dlg->Continue());
    TestDialogueText(this, "Text node", Dlg, "Player", "OK");
    TestTrue("Continue", Dlg->Continue());

    // Restart
    TestDialogueText(this, "Text node", Dlg, "NPC", "Reply when random == 1");
    TestTrue("Continue", Dlg->Continue());
    TestDialogueText(this, "Text node", Dlg, "NPC", "Second random == 2");
    TestTrue("Continue", Dlg->Continue());
    TestDialogueText(this, "Text node", Dlg, "Player", "OK");
    TestTrue("Continue", Dlg->Continue());

    // Restart
    TestDialogueText(this, "Text node", Dlg, "NPC", "Reply when random == 0");
    TestTrue("Continue", Dlg->Continue());
    TestDialogueText(this, "Text node", Dlg, "NPC", "Second random == 0");
    TestTrue("Continue", Dlg->Continue());
    TestDialogueText(this, "Text node", Dlg, "Player", "OK");
    TestTrue("Continue", Dlg->Continue());
    
    // Restart
    TestDialogueText(this, "Text node", Dlg, "NPC", "Reply when random == 2");
    TestTrue("Continue", Dlg->Continue());
    TestDialogueText(this, "Text node", Dlg, "NPC", "Second random == 2");
    TestTrue("Continue", Dlg->Continue());
    TestDialogueText(this, "Text node", Dlg, "Player", "OK");
    TestTrue("Continue", Dlg->Continue());
    
    Script->MarkAsGarbage();
    return true;
    
}


const FString MixedConditionalChoiceAndRandomInput = R"RAWSUD(
Player: Hello
    * First choice
        Player: I took the 1.1 choice
[if {x} > 0]
    * Second choice (conditional)
        [random]
            Player: I took the 1.2 choice, random == 0
        [or]
            Player: I took the 1.2 choice, random == 1
        [endif]
    * Third choice (conditional)
        Player: I took the 1.3 choice
[else]
    * Second Alt Choice
        [random]
            Player: I took the alt 1.2 choice, random == 0
        [or]
            Player: I took the alt 1.2 choice, random == 1
        [endif]
[endif]
    * Common last choice
        Player: I took the 1.4 choice


)RAWSUD";

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestRandomMixed,
                                 "SUDSTest.TestRandomMixed",
                                 EAutomationTestFlags::EditorContext |
                                 EAutomationTestFlags::ClientContext |
                                 EAutomationTestFlags::ProductFilter)
bool FTestRandomMixed::RunTest(const FString& Parameters)
{
    FSUDSMessageLogger Logger(false);
    FSUDSScriptImporter Importer;
    TestTrue("Import should succeed", Importer.ImportFromBuffer(GetData(MixedConditionalChoiceAndRandomInput), MixedConditionalChoiceAndRandomInput.Len(), "MixedRandomInput", &Logger, true));

    auto Script = NewObject<USUDSScript>(GetTransientPackage(), "Test");
    const ScopedStringTableHolder StringTableHolder;
    Importer.PopulateAsset(Script, StringTableHolder.StringTable);

    // Script shouldn't be the owner of the dialogue but it's the only UObject we've got right now so why not
    auto Dlg = USUDSLibrary::CreateDialogue(Script, Script);

    // Seed random so we have consistent results
    FMath::SRandInit(999);
    Dlg->SetVariableInt("x", 5);
    Dlg->Start();

    TestDialogueText(this, "Start node", Dlg, "Player", "Hello");
    // 2 choices from conditional, 2 common
    if (TestEqual("Choice count when x=5", Dlg->GetNumberOfChoices(), 4))
    {
        TestTrue("Choose 1", Dlg->Choose(1));
        TestDialogueText(this, "Text node", Dlg, "Player", "I took the 1.2 choice, random == 0");
    }

    // Restart, same again but different result if random
    Dlg->Restart(false);
    TestDialogueText(this, "Start node", Dlg, "Player", "Hello");
    // 2 choices from conditional, 2 common
    if (TestEqual("Choice count when x=5", Dlg->GetNumberOfChoices(), 4))
    {
        TestTrue("Choose 1", Dlg->Choose(1));
        TestDialogueText(this, "Text node", Dlg, "Player", "I took the 1.2 choice, random == 1");
    }

    // Now change to other conditional path
    Dlg->SetVariableInt("x", 0);

    Dlg->Restart(false);
    TestDialogueText(this, "Start node", Dlg, "Player", "Hello");
    if (TestEqual("Choice count when x=0", Dlg->GetNumberOfChoices(), 3))
    {
        TestTrue("Choose 1", Dlg->Choose(1));
        TestDialogueText(this, "Text node", Dlg, "Player", "I took the alt 1.2 choice, random == 1");
    }
    Dlg->Restart(false);
    TestDialogueText(this, "Start node", Dlg, "Player", "Hello");
    if (TestEqual("Choice count when x=0", Dlg->GetNumberOfChoices(), 3))
    {
        TestTrue("Choose 1", Dlg->Choose(1));
        TestDialogueText(this, "Text node", Dlg, "Player", "I took the alt 1.2 choice, random == 0");
    }

    
    
    Script->MarkAsGarbage();
    return true;
    
}

UE_ENABLE_OPTIMIZATION