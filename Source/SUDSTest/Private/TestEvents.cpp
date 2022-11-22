#include "SUDSDialogue.h"
#include "SUDSLibrary.h"
#include "SUDSScript.h"
#include "SUDSScriptImporter.h"
#include "TestEventSub.h"
#include "TestParticipant.h"
#include "TestUtils.h"
#include "Internationalization/StringTable.h"
#include "Internationalization/StringTableRegistry.h"

PRAGMA_DISABLE_OPTIMIZATION

const FString EventParsingInput = R"RAWSUD(
Player: Ow do?
[set IntVar 2]
[set FloatVar 66.67]
[set StringVar "Ey up"]
[event SummatHappened "2 penneth", 99.99, masculine, {IntVar}, 42, false]
NPC: Alreet chook
[event WellBlowMeDown {FloatVar}, true, {StringVar}]
[event Calculated {FloatVar} + 10, true or false, {StringVar}]
Player: Tara
)RAWSUD";

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestEvents,
								 "SUDSTest.TestEvents",
								 EAutomationTestFlags::EditorContext |
								 EAutomationTestFlags::ClientContext |
								 EAutomationTestFlags::ProductFilter)


bool FTestEvents::RunTest(const FString& Parameters)
{
	FSUDSScriptImporter Importer;
	TestTrue("Import should succeed", Importer.ImportFromBuffer(GetData(EventParsingInput), EventParsingInput.Len(), "EventParsingInput", true));

	auto Script = NewObject<USUDSScript>(GetTransientPackage(), "Test");
	const ScopedStringTableHolder StringTableHolder;
	Importer.PopulateAsset(Script, StringTableHolder.StringTable);

	// Script shouldn't be the owner of the dialogue but it's the only UObject we've got right now so why not
	auto Dlg = USUDSLibrary::CreateDialogue(Script, Script);

	// Subscriber
	auto EvtSub = NewObject<UTestEventSub>();
	EvtSub->Init(Dlg);

	// Participant
	auto Participant = NewObject<UTestParticipant>();
	Participant->TestNumber = 0;
	Dlg->AddParticipant(Participant);

	Dlg->Start();

	TestDialogueText(this, "Line 1", Dlg, "Player", "Ow do?");
	TestTrue("Continue", Dlg->Continue());

	// Should have had an event
	if (TestEqual("Event sub should have received", EvtSub->EventRecords.Num(), 1))
	{
		TestEqual("Event name", EvtSub->EventRecords[0].Name.ToString(), "SummatHappened");
		if (TestEqual("Event sub arg count", EvtSub->EventRecords[0].Args.Num(), 6))
		{
			TestEqual("Event sub arg 0 type", EvtSub->EventRecords[0].Args[0].GetType(), ESUDSValueType::Text);
			TestEqual("Event sub arg 0 value", EvtSub->EventRecords[0].Args[0].GetTextValue().ToString(), "2 penneth");
			TestEqual("Event sub arg 1 type", EvtSub->EventRecords[0].Args[1].GetType(), ESUDSValueType::Float);
			TestEqual("Event sub arg 1 value", EvtSub->EventRecords[0].Args[1].GetFloatValue(), 99.99f);
			TestEqual("Event sub arg 2 type", EvtSub->EventRecords[0].Args[2].GetType(), ESUDSValueType::Gender);
			TestEqual("Event sub arg 2 value", EvtSub->EventRecords[0].Args[2].GetGenderValue(), ETextGender::Masculine);
			TestEqual("Event sub arg 3 type", EvtSub->EventRecords[0].Args[3].GetType(), ESUDSValueType::Int);
			TestEqual("Event sub arg 3 value", EvtSub->EventRecords[0].Args[3].GetIntValue(), 2);
			TestEqual("Event sub arg 4 type", EvtSub->EventRecords[0].Args[4].GetType(), ESUDSValueType::Int);
			TestEqual("Event sub arg 4 value", EvtSub->EventRecords[0].Args[4].GetIntValue(), 42);
			TestEqual("Event sub arg 5 type", EvtSub->EventRecords[0].Args[5].GetType(), ESUDSValueType::Boolean);
			TestEqual("Event sub arg 5 value", EvtSub->EventRecords[0].Args[5].GetBooleanValue(), false);
		}
	}
	if (TestEqual("Participant should have received", Participant->EventRecords.Num(), 1))
	{
		TestEqual("Event name", Participant->EventRecords[0].Name.ToString(), "SummatHappened");
		if (TestEqual("Participant arg count", Participant->EventRecords[0].Args.Num(), 6))
		{
			TestEqual("Participant arg 0 type", Participant->EventRecords[0].Args[0].GetType(), ESUDSValueType::Text);
			TestEqual("Participant arg 0 value", Participant->EventRecords[0].Args[0].GetTextValue().ToString(), "2 penneth");
			TestEqual("Participant arg 1 type", Participant->EventRecords[0].Args[1].GetType(), ESUDSValueType::Float);
			TestEqual("Participant arg 1 value", Participant->EventRecords[0].Args[1].GetFloatValue(), 99.99f);
			TestEqual("Participant arg 2 type", Participant->EventRecords[0].Args[2].GetType(), ESUDSValueType::Gender);
			TestEqual("Participant arg 2 value", Participant->EventRecords[0].Args[2].GetGenderValue(), ETextGender::Masculine);
			TestEqual("Participant arg 3 type", Participant->EventRecords[0].Args[3].GetType(), ESUDSValueType::Int);
			TestEqual("Participant arg 3 value", Participant->EventRecords[0].Args[3].GetIntValue(), 2);
			TestEqual("Participant arg 4 type", Participant->EventRecords[0].Args[4].GetType(), ESUDSValueType::Int);
			TestEqual("Participant arg 4 value", Participant->EventRecords[0].Args[4].GetIntValue(), 42);
			TestEqual("Participant arg 5 type", Participant->EventRecords[0].Args[5].GetType(), ESUDSValueType::Boolean);
			TestEqual("Participant arg 5 value", Participant->EventRecords[0].Args[5].GetBooleanValue(), false);
			
		}
		
	}

	TestDialogueText(this, "Line 2", Dlg, "NPC", "Alreet chook");
	TestTrue("Continue", Dlg->Continue());

	if (TestEqual("Event sub should have received", EvtSub->EventRecords.Num(), 3))
	{
		TestEqual("Event name", EvtSub->EventRecords[1].Name.ToString(), "WellBlowMeDown");
		if (TestEqual("Event sub arg count", EvtSub->EventRecords[1].Args.Num(), 3))
		{
			TestEqual("Event sub arg 0 type", EvtSub->EventRecords[1].Args[0].GetType(), ESUDSValueType::Float);
			TestEqual("Event sub arg 0 value", EvtSub->EventRecords[1].Args[0].GetFloatValue(), 66.67f);
			TestEqual("Event sub arg 1 type", EvtSub->EventRecords[1].Args[1].GetType(), ESUDSValueType::Boolean);
			TestEqual("Event sub arg 1 value", EvtSub->EventRecords[1].Args[1].GetBooleanValue(), true);
			TestEqual("Event sub arg 2 type", EvtSub->EventRecords[1].Args[2].GetType(), ESUDSValueType::Text);
			TestEqual("Event sub arg 2 value", EvtSub->EventRecords[1].Args[2].GetTextValue().ToString(), "Ey up");
		}
		TestEqual("Event name", EvtSub->EventRecords[2].Name.ToString(), "Calculated");
		if (TestEqual("Event sub arg count", EvtSub->EventRecords[2].Args.Num(), 3))
		{
			TestEqual("Event sub arg 0 type", EvtSub->EventRecords[2].Args[0].GetType(), ESUDSValueType::Float);
			TestEqual("Event sub arg 0 value", EvtSub->EventRecords[2].Args[0].GetFloatValue(), 76.67f);
			TestEqual("Event sub arg 2 type", EvtSub->EventRecords[2].Args[1].GetType(), ESUDSValueType::Boolean);
			TestEqual("Event sub arg 2 value", EvtSub->EventRecords[2].Args[1].GetBooleanValue(), true);
			TestEqual("Event sub arg 2 type", EvtSub->EventRecords[2].Args[2].GetType(), ESUDSValueType::Text);
			TestEqual("Event sub arg 2 value", EvtSub->EventRecords[2].Args[2].GetTextValue().ToString(), "Ey up");
		}
	}
	if (TestEqual("Participant should have received", Participant->EventRecords.Num(), 3))
	{
		TestEqual("Event name", Participant->EventRecords[1].Name.ToString(), "WellBlowMeDown");
		if (TestEqual("Participant arg count", Participant->EventRecords[1].Args.Num(), 3))
		{
			TestEqual("Participant arg 0 type", Participant->EventRecords[1].Args[0].GetType(), ESUDSValueType::Float);
			TestEqual("Participant arg 0 value", Participant->EventRecords[1].Args[0].GetFloatValue(), 66.67f);
			TestEqual("Participant arg 1 type", Participant->EventRecords[1].Args[1].GetType(), ESUDSValueType::Boolean);
			TestEqual("Participant arg 1 value", Participant->EventRecords[1].Args[1].GetBooleanValue(), true);
			TestEqual("Participant arg 2 type", Participant->EventRecords[1].Args[2].GetType(), ESUDSValueType::Text);
			TestEqual("Participant arg 2 value", Participant->EventRecords[1].Args[2].GetTextValue().ToString(), "Ey up");
		}
		TestEqual("Event name", Participant->EventRecords[2].Name.ToString(), "Calculated");
		if (TestEqual("Event sub arg count", Participant->EventRecords[2].Args.Num(), 3))
		{
			TestEqual("Event sub arg 0 type", Participant->EventRecords[2].Args[0].GetType(), ESUDSValueType::Float);
			TestEqual("Event sub arg 0 value", Participant->EventRecords[2].Args[0].GetFloatValue(), 76.67f);
			TestEqual("Event sub arg 2 type", Participant->EventRecords[2].Args[1].GetType(), ESUDSValueType::Boolean);
			TestEqual("Event sub arg 2 value", Participant->EventRecords[2].Args[1].GetBooleanValue(), true);
			TestEqual("Event sub arg 2 type", Participant->EventRecords[2].Args[2].GetType(), ESUDSValueType::Text);
			TestEqual("Event sub arg 2 value", Participant->EventRecords[2].Args[2].GetTextValue().ToString(), "Ey up");
		}
		
	}
	
	
	return true;
}

PRAGMA_ENABLE_OPTIMIZATION