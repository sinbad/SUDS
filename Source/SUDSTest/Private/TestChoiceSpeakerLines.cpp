#include "SUDSDialogue.h"
#include "SUDSLibrary.h"
#include "SUDSMessageLogger.h"
#include "SUDSScript.h"
#include "SUDSScriptImporter.h"
#include "TestParticipant.h"
#include "TestUtils.h"
#include "Misc/AutomationTest.h"

UE_DISABLE_OPTIMIZATION

const FString ChoicesAsSpeakerLineEnableInScriptInput = R"RAWSUD(
===
[importsetting GenerateSpeakerLinesFromChoices true]
===
Player: Hello
  * Choice 1
	NPC: I see
		* Choice 1a
            NPC: Sure
        *- Not a speaker line!
			Player: I had to say this separately
  * Choice 2
	NPC: Totally

Player: The end
)RAWSUD";



IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestChoiceSpeakerLineInScript,
								 "SUDSTest.TestChoiceSpeakerLineInScript",
								 EAutomationTestFlags::EditorContext |
								 EAutomationTestFlags::ClientContext |
								 EAutomationTestFlags::ProductFilter)



bool FTestChoiceSpeakerLineInScript::RunTest(const FString& Parameters)
{
	FSUDSMessageLogger Logger(false);
	FSUDSScriptImporter Importer;
	TestTrue("Import should succeed", Importer.ImportFromBuffer(GetData(ChoicesAsSpeakerLineEnableInScriptInput), ChoicesAsSpeakerLineEnableInScriptInput.Len(), "ChoicesAsSpeakerLineEnableInScriptInput", &Logger, true));

	auto Script = NewObject<USUDSScript>(GetTransientPackage(), "Test");
	const ScopedStringTableHolder StringTableHolder;
	Importer.PopulateAsset(Script, StringTableHolder.StringTable);

	// Script shouldn't be the owner of the dialogue but it's the only UObject we've got right now so why not
	auto Dlg = USUDSLibrary::CreateDialogue(Script, Script);
	auto Participant = NewObject<UTestParticipant>();
	Participant->TestNumber = 0;
	Dlg->AddParticipant(Participant);
	Dlg->Start();

	TestDialogueText(this, "Line 1", Dlg, "Player", "Hello");
	TestEqual("Choice text", Dlg->GetChoiceText(0).ToString(), TEXT("Choice 1"));
	TestEqual("Choice text", Dlg->GetChoiceText(1).ToString(), TEXT("Choice 2"));
	const FText ChoiceText0 = Dlg->GetChoiceText(0);
	Dlg->Choose(0);
	// Confirm that we got the choice as a speaker line
	TestDialogueText(this, "Choice as speaker line 1", Dlg, "Player", "Choice 1");
	// Confirm that text ID is the same
	TestTrue("Text should be identical", ChoiceText0.IdenticalTo(Dlg->GetText(), ETextIdenticalModeFlags::DeepCompare));
	TestEqual("TextIDs should match", FTextInspector::GetTextId(ChoiceText0).GetKey().GetChars(), FTextInspector::GetTextId(Dlg->GetText()).GetKey().GetChars());
	
	Dlg->Continue();
	TestDialogueText(this, "Regular speaker line", Dlg, "NPC", "I see");
	TestEqual("Choice text", Dlg->GetChoiceText(0).ToString(), TEXT("Choice 1a"));
	TestEqual("Choice text", Dlg->GetChoiceText(1).ToString(), TEXT("Not a speaker line!"));

	// Test choice which disables speaker line
	Dlg->Choose(1);
	TestDialogueText(this, "Regular speaker line", Dlg, "Player", "I had to say this separately");
	
	
	Script->MarkAsGarbage();
	return true;
}


const FString ChoicesAsSpeakerLineSetSpeakerIdInScriptInput = R"RAWSUD(
===
[importsetting GenerateSpeakerLinesFromChoices true]
[importsetting SpeakerIDForGeneratedLinesFromChoices `TheDude`]
===
Player: Hello
  * Choice 1
	NPC: You said that choice
  * Choice 2
	NPC: Also that one
Player: The end
)RAWSUD";



IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestChoiceSpeakerSetSpeakerIdInScriptInput,
								 "SUDSTest.TestChoiceSpeakerSetSpeakerIdInScriptInput",
								 EAutomationTestFlags::EditorContext |
								 EAutomationTestFlags::ClientContext |
								 EAutomationTestFlags::ProductFilter)



bool FTestChoiceSpeakerSetSpeakerIdInScriptInput::RunTest(const FString& Parameters)
{
	FSUDSMessageLogger Logger(false);
	FSUDSScriptImporter Importer;
	TestTrue("Import should succeed", Importer.ImportFromBuffer(GetData(ChoicesAsSpeakerLineSetSpeakerIdInScriptInput), ChoicesAsSpeakerLineSetSpeakerIdInScriptInput.Len(), "ChoicesAsSpeakerLineSetSpeakerIdInScriptInput", &Logger, true));

	auto Script = NewObject<USUDSScript>(GetTransientPackage(), "Test");
	const ScopedStringTableHolder StringTableHolder;
	Importer.PopulateAsset(Script, StringTableHolder.StringTable);

	// Script shouldn't be the owner of the dialogue but it's the only UObject we've got right now so why not
	auto Dlg = USUDSLibrary::CreateDialogue(Script, Script);
	auto Participant = NewObject<UTestParticipant>();
	Participant->TestNumber = 0;
	Dlg->AddParticipant(Participant);
	Dlg->Start();

	TestDialogueText(this, "Line 1", Dlg, "Player", "Hello");
	TestEqual("Choice text", Dlg->GetChoiceText(0).ToString(), TEXT("Choice 1"));
	TestEqual("Choice text", Dlg->GetChoiceText(1).ToString(), TEXT("Choice 2"));
	const FText ChoiceText0 = Dlg->GetChoiceText(0);
	Dlg->Choose(0);
	// Confirm that text ID is the same
	TestTrue("Text should be identical", ChoiceText0.IdenticalTo(Dlg->GetText(), ETextIdenticalModeFlags::DeepCompare));
	TestEqual("TextIDs should match", FTextInspector::GetTextId(ChoiceText0).GetKey().GetChars(), FTextInspector::GetTextId(Dlg->GetText()).GetKey().GetChars());
	
	// Confirm that we got the choice as a speaker line, with custom speaker ID
	TestDialogueText(this, "Choice as speaker line 1", Dlg, "TheDude", "Choice 1");
	Dlg->Continue();
	TestDialogueText(this, "Regular speaker line", Dlg, "NPC", "You said that choice");
	
	Script->MarkAsGarbage();
	return true;
}

const FString ChoicesAsSpeakerLineChainChoiceInput = R"RAWSUD(
===
[importsetting GenerateSpeakerLinesFromChoices true]
===
Player: Hello
  * Choice 1
	* Choice 1a
		NPC: Hey this worked
	* Choice 1b
  * Choice 2
	NPC: Also that one
Player: The end
)RAWSUD";



IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestChoiceSpeakerChainedChoiceInput,
								 "SUDSTest.TestChoiceSpeakerChainedChoiceInput",
								 EAutomationTestFlags::EditorContext |
								 EAutomationTestFlags::ClientContext |
								 EAutomationTestFlags::ProductFilter)



bool FTestChoiceSpeakerChainedChoiceInput::RunTest(const FString& Parameters)
{
	FSUDSMessageLogger Logger(false);
	FSUDSScriptImporter Importer;
	TestTrue("Import should succeed", Importer.ImportFromBuffer(GetData(ChoicesAsSpeakerLineChainChoiceInput), ChoicesAsSpeakerLineChainChoiceInput.Len(), "ChoicesAsSpeakerLineChainChoiceInput", &Logger, true));

	auto Script = NewObject<USUDSScript>(GetTransientPackage(), "Test");
	const ScopedStringTableHolder StringTableHolder;
	Importer.PopulateAsset(Script, StringTableHolder.StringTable);

	// Script shouldn't be the owner of the dialogue but it's the only UObject we've got right now so why not
	auto Dlg = USUDSLibrary::CreateDialogue(Script, Script);
	auto Participant = NewObject<UTestParticipant>();
	Participant->TestNumber = 0;
	Dlg->AddParticipant(Participant);
	Dlg->Start();

	TestDialogueText(this, "Line 1", Dlg, "Player", "Hello");
	TestEqual("Choice text", Dlg->GetChoiceText(0).ToString(), TEXT("Choice 1"));
	TestEqual("Choice text", Dlg->GetChoiceText(1).ToString(), TEXT("Choice 2"));
	Dlg->Choose(0);
	// Confirm that we got the choice as a speaker line, with custom speaker ID
	TestDialogueText(this, "Choice as speaker line 1", Dlg, "Player", "Choice 1");
	TestEqual("Choice 2nd level text", Dlg->GetChoiceText(0).ToString(), TEXT("Choice 1a"));
	TestEqual("Choice 2nd leveltext", Dlg->GetChoiceText(1).ToString(), TEXT("Choice 1b"));
	Dlg->Choose(0);
	TestDialogueText(this, "Choice as speaker line 1", Dlg, "Player", "Choice 1a");
	Dlg->Continue();
	TestDialogueText(this, "Regular speaker line", Dlg, "NPC", "Hey this worked");
	
	Script->MarkAsGarbage();
	return true;
}

const FString ChoicesAsSpeakerLineWithStringKeysInput = R"RAWSUD(
===
[importsetting GenerateSpeakerLinesFromChoices true]
===
Player: Hello @0012@
  * Choice 1 @0016@
	NPC: I see @0017@
		* Choice 1a @0018@
            NPC: Sure
        *- Not a speaker line!
			Player: I had to say this separately
  * Choice 2
	NPC: Totally

Player: The end
)RAWSUD";



IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestChoicesAsSpeakerLineWithStringKeys,
								 "SUDSTest.TestChoicesAsSpeakerLineWithStringKeys",
								 EAutomationTestFlags::EditorContext |
								 EAutomationTestFlags::ClientContext |
								 EAutomationTestFlags::ProductFilter)



bool FTestChoicesAsSpeakerLineWithStringKeys::RunTest(const FString& Parameters)
{
	FSUDSMessageLogger Logger(false);
	FSUDSScriptImporter Importer;
	TestTrue("Import should succeed", Importer.ImportFromBuffer(GetData(ChoicesAsSpeakerLineWithStringKeysInput), ChoicesAsSpeakerLineWithStringKeysInput.Len(), "ChoicesAsSpeakerLineWithStringKeysInput", &Logger, true));

	auto Script = NewObject<USUDSScript>(GetTransientPackage(), "Test");
	const ScopedStringTableHolder StringTableHolder;
	Importer.PopulateAsset(Script, StringTableHolder.StringTable);

	// Script shouldn't be the owner of the dialogue but it's the only UObject we've got right now so why not
	auto Dlg = USUDSLibrary::CreateDialogue(Script, Script);
	auto Participant = NewObject<UTestParticipant>();
	Participant->TestNumber = 0;
	Dlg->AddParticipant(Participant);
	Dlg->Start();

	TestDialogueText(this, "Line 1", Dlg, "Player", "Hello");
	TestEqual("String key", FTextInspector::GetTextId(Dlg->GetText()).GetKey().GetChars(), "@0012@");
	TestEqual("Choice text", Dlg->GetChoiceText(0).ToString(), TEXT("Choice 1"));
	TestEqual("Choice text", Dlg->GetChoiceText(1).ToString(), TEXT("Choice 2"));
	TestEqual("String key", FTextInspector::GetTextId(Dlg->GetChoiceText(0)).GetKey().GetChars(), "@0016@");
	FText ChoiceText0 = Dlg->GetChoiceText(0);
	Dlg->Choose(0);
	// Confirm that we got the choice as a speaker line
	TestDialogueText(this, "Choice as speaker line 1", Dlg, "Player", "Choice 1");
	// Confirm that text ID is the same
	TestTrue("Text should be identical", ChoiceText0.IdenticalTo(Dlg->GetText(), ETextIdenticalModeFlags::DeepCompare));
	TestEqual("TextIDs should match", FTextInspector::GetTextId(ChoiceText0).GetKey().GetChars(), FTextInspector::GetTextId(Dlg->GetText()).GetKey().GetChars());
	
	Dlg->Continue();
	TestDialogueText(this, "Regular speaker line", Dlg, "NPC", "I see");
	TestEqual("String key", FTextInspector::GetTextId(Dlg->GetText()).GetKey().GetChars(), "@0017@");
	TestEqual("String key", FTextInspector::GetTextId(Dlg->GetChoiceText(0)).GetKey().GetChars(), "@0018@");
	TestEqual("Choice text", Dlg->GetChoiceText(0).ToString(), TEXT("Choice 1a"));
	TestEqual("Choice text", Dlg->GetChoiceText(1).ToString(), TEXT("Not a speaker line!"));

	ChoiceText0 = Dlg->GetChoiceText(0);
	Dlg->Choose(0);
	TestDialogueText(this, "Duplicated speaker line", Dlg, "Player", "Choice 1a");
	
	// Confirm that text ID is the same
	TestTrue("Text should be identical", ChoiceText0.IdenticalTo(Dlg->GetText(), ETextIdenticalModeFlags::DeepCompare));
	TestEqual("TextIDs should match", FTextInspector::GetTextId(ChoiceText0).GetKey().GetChars(), FTextInspector::GetTextId(Dlg->GetText()).GetKey().GetChars());

	Script->MarkAsGarbage();
	return true;
}

UE_ENABLE_OPTIMIZATION