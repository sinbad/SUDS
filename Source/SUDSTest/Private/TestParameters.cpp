#include "SUDSDialogue.h"
#include "SUDSLibrary.h"
#include "SUDSScript.h"
#include "SUDSScriptImporter.h"
#include "TestParticipant.h"
#include "TestUtils.h"
#include "Internationalization/StringTable.h"
#include "Internationalization/StringTableRegistry.h"
#include "Misc/AutomationTest.h"

PRAGMA_DISABLE_OPTIMIZATION

const FString ParamsInput = R"RAWSUD(
Player: Hello, I'm {PlayerName}
NPC: Greetings, {PlayerName}, my name is {NPCName}
Player: My friend's name is {FriendName}, {Gender}|gender(he,she,they) {Gender}|gender(has,has,have) {NumCats} {NumCats}|plural(one=cat,other=cats)
NPC: Floating point {FloatVal} format test
Player: Boolean test {BoolVal}?
	* Choose, {PlayerName}!
	* Is {NumCats} {NumCats}|plural(one=cat,other=cats) too many?
Player: Nothing
)RAWSUD";



IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestParameters,
								 "SUDSTest.TestParameters",
								 EAutomationTestFlags::EditorContext |
								 EAutomationTestFlags::ClientContext |
								 EAutomationTestFlags::ProductFilter)



bool FTestParameters::RunTest(const FString& Parameters)
{
	
	FSUDSScriptImporter Importer;
	TestTrue("Import should succeed", Importer.ImportFromBuffer(GetData(ParamsInput), ParamsInput.Len(), "ParamsInput", true));

	auto Script = NewObject<USUDSScript>(GetTransientPackage(), "Test");
	auto StringTable = NewObject<UStringTable>(GetTransientPackage(), "TestStrings");
	Importer.PopulateAsset(Script, StringTable);

	// Script shouldn't be the owner of the dialogue but it's the only UObject we've got right now so why not
	auto Dlg = USUDSLibrary::CreateDialogue(Script, Script);
	auto Participant = NewObject<UTestParticipant>();
	Participant->TestNumber = 0;
	Dlg->AddParticipant("Dummy", Participant);

	TestDialogueText(this, "Line 1", Dlg, "Player", "Hello, I'm Protagonist");
	Dlg->Continue();
	TestDialogueText(this, "Line 2", Dlg, "NPC", "Greetings, Protagonist, my name is An NPC");
	Dlg->Continue();
	TestDialogueText(this, "Line 3", Dlg, "Player", "My friend's name is Susan, she has 3 cats");
	Dlg->Continue();
	TestDialogueText(this, "Line 4", Dlg, "NPC", "Floating point 12.567 format test");
	Dlg->Continue();
	TestDialogueText(this, "Line 5", Dlg, "Player", "Boolean test 1?");
	if (TestEqual("Number of choices", Dlg->GetNumberOfChoices(), 2))
	{
		TestEqual("Choice text 1", Dlg->GetChoiceText(0).ToString(), "Choose, Protagonist!");
		TestEqual("Choice text 2", Dlg->GetChoiceText(1).ToString(), "Is 3 cats too many?");

	}
	

	// Tidy up string table
	// Constructor registered this table
	FStringTableRegistry::Get().UnregisterStringTable(StringTable->GetStringTableId());
	
	return true;
}

PRAGMA_ENABLE_OPTIMIZATION