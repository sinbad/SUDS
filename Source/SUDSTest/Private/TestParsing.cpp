#include "Misc/AutomationTest.h"
#include "SUDSScriptImporter.h"

const FString SimpleParsingInput = R"RAWSUD(
===
# Nothing in header but a comment
===

# A comment in body

Player: Excuse me?
NPC: Well, hello there. This is a test.
  * A test?
    NPC: Yes, a test. This is some indented continuation text.
    Player: Oh I see, thank you.
	NPC: You're welcome.
  * Another option
    NPC: This is another option with an embedded continuation.
    * How far can this go?
      NPC: Theoretically forever but who knows?
	* This is an extra question
	  NPC: That should have been added to the previous choice
	* Another question?
	  NPC: Yep, this one too
Player: Well, that's all for now. This should appear for all paths as a fall-through.
	This, in fact, is a multi-line piece of text
	Which is joined to the previous text node with the line breaks
NPC: Bye!
)RAWSUD";

const FString ConditionalParsingInput = R"RAWSUD(
===
# Nothing in header but a comment
===

# A comment in body

Player: Excuse me?
NPC: Well, hello there. This is a test.
  * A test?
    NPC: Yes, a test. This is some indented continuation text.
    Player: Oh I see, thank you.
	[if $polite == 1]
		NPC: You're welcome.
    [else]
		NPC: Too right.
    [endif]
  * Another option
    NPC: This is another option with an embedded continuation.
    * How far can this go?
      NPC: Theoretically forever but who knows?
	[if $extraq]
		* This is an extra question
		  NPC: That should have been added to the previous choice
		* Another question?
		  NPC: Yep, this one too
	[endif]
Player: Well, that's all for now. This should appear for all paths as a fall-through.
	This, in fact, is a multi-line piece of text
	Which is joined to the previous text node with the line breaks
NPC: Bye!
)RAWSUD";

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestSimpleParsing,
                                 "SUDSTest.TestSimpleParsing",
                                 EAutomationTestFlags::EditorContext |
                                 EAutomationTestFlags::ClientContext |
                                 EAutomationTestFlags::ProductFilter)


bool FTestSimpleParsing::RunTest(const FString& Parameters)
{
	FSUDSScriptImporter Importer;
	TestTrue("Import should succeed", Importer.ImportFromBuffer(GetData(SimpleParsingInput), SimpleParsingInput.Len(), "SimpleParsingInput", true));

	// Test the content of the parsing
	auto RootNode = Importer.GetNode(0);
	TestNotNull("Root node should exist", RootNode);
	TestEqual("Root node type", RootNode->NodeType, ESUDSScriptNodeType::Text);
	TestEqual("Root node speaker", RootNode->Speaker, "Player");
	TestEqual("Root node text", RootNode->Text, "Excuse me?");
	TestEqual("Root node edges", RootNode->Edges.Num(), 1);

	TestFalse("First node edge not jump", RootNode->Edges[0].bIsJump);
	auto NextNode = Importer.GetNode(RootNode->Edges[0].TargetNodeIdx);
	TestNotNull("Next node should exist", NextNode);

	TestEqual("Second node type", NextNode->NodeType, ESUDSScriptNodeType::Text);
	TestEqual("Second node speaker", NextNode->Speaker, "NPC");
	TestEqual("Second node text", NextNode->Text, "Well, hello there. This is a test.");
	TestEqual("Second node edges", NextNode->Edges.Num(), 1);

	TestFalse("Second node edge not jump", NextNode->Edges[0].bIsJump);
	NextNode = Importer.GetNode(NextNode->Edges[0].TargetNodeIdx);
	TestNotNull("Third node should exist", NextNode);
	TestEqual("Third node type", NextNode->NodeType, ESUDSScriptNodeType::Choice);
	TestEqual("Third node edges", NextNode->Edges.Num(), 2);

	return true;
}
