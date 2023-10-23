#include "SUDSDialogue.h"
#include "SUDSLibrary.h"
#include "SUDSMessageLogger.h"
#include "SUDSScript.h"
#include "SUDSScriptImporter.h"
#include "TestEventSub.h"
#include "TestParticipant.h"
#include "TestUtils.h"

UE_DISABLE_OPTIMIZATION

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
	FSUDSMessageLogger Logger(false);
	FSUDSScriptImporter Importer;
	TestTrue("Import should succeed", Importer.ImportFromBuffer(GetData(EventParsingInput), EventParsingInput.Len(), "EventParsingInput", &Logger, true));

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
	if (TestEqual("Event var sub should have received", EvtSub->SetVarRecords.Num(), 10))
	{
		// First 7 are from the participant setting variables at startup
		TestEqual("Event var sub arg 0 name", EvtSub->SetVarRecords[0].Name.ToString(), "SpeakerName.Player");
		TestEqual("Event var sub arg 0 type", EvtSub->SetVarRecords[0].Value.GetType(), ESUDSValueType::Text);
		TestEqual("Event var sub arg 0 value", EvtSub->SetVarRecords[0].Value.GetTextValue().ToString(), "Protagonist");
		TestFalse("Event var sub arg 0 fromscript", EvtSub->SetVarRecords[0].bFromScript);

		TestEqual("Event var sub arg 1 name", EvtSub->SetVarRecords[1].Name.ToString(), "SpeakerName.NPC");
		TestEqual("Event var sub arg 1 type", EvtSub->SetVarRecords[1].Value.GetType(), ESUDSValueType::Text);
		TestEqual("Event var sub arg 1 value", EvtSub->SetVarRecords[1].Value.GetTextValue().ToString(), "An NPC");
		TestFalse("Event var sub arg 1 fromscript", EvtSub->SetVarRecords[1].bFromScript);

		TestEqual("Event var sub arg 2 name", EvtSub->SetVarRecords[2].Name.ToString(), "NumCats");
		TestEqual("Event var sub arg 2 type", EvtSub->SetVarRecords[2].Value.GetType(), ESUDSValueType::Int);
		TestEqual("Event var sub arg 2 value", EvtSub->SetVarRecords[2].Value.GetIntValue(), 3);
		TestFalse("Event var sub arg 2 fromscript", EvtSub->SetVarRecords[2].bFromScript);

		TestEqual("Event var sub arg 3 name", EvtSub->SetVarRecords[3].Name.ToString(), "FriendName");
		TestEqual("Event var sub arg 3 type", EvtSub->SetVarRecords[3].Value.GetType(), ESUDSValueType::Text);
		TestEqual("Event var sub arg 3 value", EvtSub->SetVarRecords[3].Value.GetTextValue().ToString(), "Susan");
		TestFalse("Event var sub arg 3 fromscript", EvtSub->SetVarRecords[3].bFromScript);

		TestEqual("Event var sub arg 4 name", EvtSub->SetVarRecords[4].Name.ToString(), "Gender");
		TestEqual("Event var sub arg 4 type", EvtSub->SetVarRecords[4].Value.GetType(), ESUDSValueType::Gender);
		TestEqual("Event var sub arg 4 value", EvtSub->SetVarRecords[4].Value.GetGenderValue(), ETextGender::Feminine);
		TestFalse("Event var sub arg 4 fromscript", EvtSub->SetVarRecords[4].bFromScript);
		
		TestEqual("Event var sub arg 5 name", EvtSub->SetVarRecords[5].Name.ToString(), "FloatVal");
		TestEqual("Event var sub arg 5 type", EvtSub->SetVarRecords[5].Value.GetType(), ESUDSValueType::Float);
		TestEqual("Event var sub arg 5 value", EvtSub->SetVarRecords[5].Value.GetFloatValue(), 12.567f);
		TestFalse("Event var sub arg 5 fromscript", EvtSub->SetVarRecords[5].bFromScript);

		TestEqual("Event var sub arg 6 name", EvtSub->SetVarRecords[6].Name.ToString(), "BoolVal");
		TestEqual("Event var sub arg 6 type", EvtSub->SetVarRecords[6].Value.GetType(), ESUDSValueType::Boolean);
		TestEqual("Event var sub arg 6 value", EvtSub->SetVarRecords[6].Value.GetBooleanValue(), true);
		TestFalse("Event var sub arg 6 fromscript", EvtSub->SetVarRecords[6].bFromScript);

		// Next 3 are from script
		TestEqual("Event var sub arg 7 name", EvtSub->SetVarRecords[7].Name.ToString(), "IntVar");
		TestEqual("Event var sub arg 7 type", EvtSub->SetVarRecords[7].Value.GetType(), ESUDSValueType::Int);
		TestEqual("Event var sub arg 7 value", EvtSub->SetVarRecords[7].Value.GetIntValue(), 2);
		TestTrue("Event var sub arg 7 fromscript", EvtSub->SetVarRecords[7].bFromScript);
		
		TestEqual("Event var sub arg 8 name", EvtSub->SetVarRecords[8].Name.ToString(), "FloatVar");
		TestEqual("Event var sub arg 8 type", EvtSub->SetVarRecords[8].Value.GetType(), ESUDSValueType::Float);
		TestEqual("Event var sub arg 8 value", EvtSub->SetVarRecords[8].Value.GetFloatValue(), 66.67f);
		TestTrue("Event var sub arg 8 fromscript", EvtSub->SetVarRecords[8].bFromScript);

		TestEqual("Event var sub arg 9 name", EvtSub->SetVarRecords[9].Name.ToString(), "StringVar");
		TestEqual("Event var sub arg 9 type", EvtSub->SetVarRecords[9].Value.GetType(), ESUDSValueType::Text);
		TestEqual("Event var sub arg 9 value", EvtSub->SetVarRecords[9].Value.GetTextValue().ToString(), "Ey up");
		TestTrue("Event var sub arg 9 fromscript", EvtSub->SetVarRecords[9].bFromScript);
	}
	if (TestEqual("Participant var should have received", Participant->SetVarRecords.Num(), 10))
	{
		// First 7 are from the participant setting variables at startup
		TestEqual("Participant var arg 0 name", Participant->SetVarRecords[0].Name.ToString(), "SpeakerName.Player");
		TestEqual("Participant var arg 0 type", Participant->SetVarRecords[0].Value.GetType(), ESUDSValueType::Text);
		TestEqual("Participant var arg 0 value", Participant->SetVarRecords[0].Value.GetTextValue().ToString(), "Protagonist");
		TestFalse("Participant var arg 0 fromscript", Participant->SetVarRecords[0].bFromScript);

		TestEqual("Participant var arg 1 name", Participant->SetVarRecords[1].Name.ToString(), "SpeakerName.NPC");
		TestEqual("Participant var arg 1 type", Participant->SetVarRecords[1].Value.GetType(), ESUDSValueType::Text);
		TestEqual("Participant var arg 1 value", Participant->SetVarRecords[1].Value.GetTextValue().ToString(), "An NPC");
		TestFalse("Participant var arg 1 fromscript", Participant->SetVarRecords[1].bFromScript);

		TestEqual("Participant var arg 2 name", Participant->SetVarRecords[2].Name.ToString(), "NumCats");
		TestEqual("Participant var arg 2 type", Participant->SetVarRecords[2].Value.GetType(), ESUDSValueType::Int);
		TestEqual("Participant var arg 2 value", Participant->SetVarRecords[2].Value.GetIntValue(), 3);
		TestFalse("Participant var arg 2 fromscript", Participant->SetVarRecords[2].bFromScript);

		TestEqual("Participant var arg 3 name", Participant->SetVarRecords[3].Name.ToString(), "FriendName");
		TestEqual("Participant var arg 3 type", Participant->SetVarRecords[3].Value.GetType(), ESUDSValueType::Text);
		TestEqual("Participant var arg 3 value", Participant->SetVarRecords[3].Value.GetTextValue().ToString(), "Susan");
		TestFalse("Participant var arg 3 fromscript", Participant->SetVarRecords[3].bFromScript);

		TestEqual("Participant var arg 4 name", Participant->SetVarRecords[4].Name.ToString(), "Gender");
		TestEqual("Participant var arg 4 type", Participant->SetVarRecords[4].Value.GetType(), ESUDSValueType::Gender);
		TestEqual("Participant var arg 4 value", Participant->SetVarRecords[4].Value.GetGenderValue(), ETextGender::Feminine);
		TestFalse("Participant var arg 4 fromscript", Participant->SetVarRecords[4].bFromScript);
		
		TestEqual("Participant var arg 5 name", Participant->SetVarRecords[5].Name.ToString(), "FloatVal");
		TestEqual("Participant var arg 5 type", Participant->SetVarRecords[5].Value.GetType(), ESUDSValueType::Float);
		TestEqual("Participant var arg 5 value", Participant->SetVarRecords[5].Value.GetFloatValue(), 12.567f);
		TestFalse("Participant var arg 5 fromscript", Participant->SetVarRecords[5].bFromScript);

		TestEqual("Participant var arg 6 name", Participant->SetVarRecords[6].Name.ToString(), "BoolVal");
		TestEqual("Participant var arg 6 type", Participant->SetVarRecords[6].Value.GetType(), ESUDSValueType::Boolean);
		TestEqual("Participant var arg 6 value", Participant->SetVarRecords[6].Value.GetBooleanValue(), true);
		TestFalse("Participant var arg 6 fromscript", Participant->SetVarRecords[6].bFromScript);

		// Next 3 are from script
		TestEqual("Participant var arg 7 name", Participant->SetVarRecords[7].Name.ToString(), "IntVar");
		TestEqual("Participant var arg 7 type", Participant->SetVarRecords[7].Value.GetType(), ESUDSValueType::Int);
		TestEqual("Participant var arg 7 value", Participant->SetVarRecords[7].Value.GetIntValue(), 2);
		TestTrue("Participant var arg 7 fromscript", Participant->SetVarRecords[7].bFromScript);
		
		TestEqual("Participant var arg 8 name", Participant->SetVarRecords[8].Name.ToString(), "FloatVar");
		TestEqual("Participant var arg 8 type", Participant->SetVarRecords[8].Value.GetType(), ESUDSValueType::Float);
		TestEqual("Participant var arg 8 value", Participant->SetVarRecords[8].Value.GetFloatValue(), 66.67f);
		TestTrue("Participant var arg 8 fromscript", Participant->SetVarRecords[8].bFromScript);

		TestEqual("Participant var arg 9 name", Participant->SetVarRecords[9].Name.ToString(), "StringVar");
		TestEqual("Participant var arg 9 type", Participant->SetVarRecords[9].Value.GetType(), ESUDSValueType::Text);
		TestEqual("Participant var arg 9 value", Participant->SetVarRecords[9].Value.GetTextValue().ToString(), "Ey up");
		TestTrue("Participant var arg 9 fromscript", Participant->SetVarRecords[9].bFromScript);
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
	
	Script->MarkAsGarbage();
	return true;
}

UE_ENABLE_OPTIMIZATION