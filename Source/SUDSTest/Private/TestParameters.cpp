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
Player: Hello, I'm {SpeakerName.Player}
NPC: Greetings, {SpeakerName.Player}, my name is {SpeakerName.NPC}
Player: My friend's name is {FriendName}, {Gender}|gender(he,she,they) {Gender}|gender(has,has,have) {NumCats} {NumCats}|plural(one=cat,other=cats)
NPC: Floating point {FloatVal} format test
Player: Boolean test {BoolVal}?
	* Choose, {SpeakerName.Player}!
	* Is {NumCats} {NumCats}|plural(one=cat,other=cats) too many?
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
	Dlg->Start();

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestParametersPriority,
								 "SUDSTest.TestParametersPriority",
								 EAutomationTestFlags::EditorContext |
								 EAutomationTestFlags::ClientContext |
								 EAutomationTestFlags::ProductFilter)



bool FTestParametersPriority::RunTest(const FString& Parameters)
{

	FSUDSScriptImporter Importer;
	TestTrue("Import should succeed", Importer.ImportFromBuffer(GetData(ParamsInput), ParamsInput.Len(), "ParamsInput", true));

	auto Script = NewObject<USUDSScript>(GetTransientPackage(), "Test");
	auto StringTable = NewObject<UStringTable>(GetTransientPackage(), "TestStrings");
	Importer.PopulateAsset(Script, StringTable);

	// Script shouldn't be the owner of the dialogue but it's the only UObject we've got right now so why not
	auto Dlg = USUDSLibrary::CreateDialogue(Script, Script);
	auto Participant1 = NewObject<UTestParticipant>();
	Participant1->TestNumber = 0; // priority 0
	auto Participant2 = NewObject<UTestParticipant>();
	Participant2->TestNumber = 1; // priority 100
	auto Participant3 = NewObject<UTestParticipant>();
	Participant1->TestNumber = 2; // priority -200
	Dlg->AddParticipant("Dummy1", Participant1);
	Dlg->AddParticipant("Dummy2", Participant2);
	Dlg->AddParticipant("Dummy3", Participant3);
	Dlg->Start();

	// Ordering of the participants should be from low to high, so variables from higher priority value participant should
	// have overridden lower priority ones

	// All of these are set by Participant 2 who is higher priority
	TestDialogueText(this, "Line 1", Dlg, "Player", "Hello, I'm Hero");
	Dlg->Continue();
	TestDialogueText(this, "Line 2", Dlg, "NPC", "Greetings, Hero, my name is Bob The NPC");
	Dlg->Continue();
	TestDialogueText(this, "Line 3", Dlg, "Player", "My friend's name is Derek, he has 5 cats");
	Dlg->Continue();

	// These are set by Participant1 and unchanged by 2. 3 has tried to set float but should have been overridden
	TestDialogueText(this, "Line 4", Dlg, "NPC", "Floating point 12.567 format test");
	Dlg->Continue();
	TestDialogueText(this, "Line 5", Dlg, "Player", "Boolean test 1?");

	// Check that there's a variable from Participant2 which no-one else set
	TestEqual("Participant3 should have set something", Dlg->GetVariableInt("SomethingUniqueTo3"), 120);
	

	// Tidy up string table
	// Constructor registered this table
	FStringTableRegistry::Get().UnregisterStringTable(StringTable->GetStringTableId());
	
	return true;	
}

PRAGMA_ENABLE_OPTIMIZATION