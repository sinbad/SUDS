#include "Misc/AutomationTest.h"
#include "SUDSScriptImporter.h"

PRAGMA_DISABLE_OPTIMIZATION

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
    NPC: This is another option with an embedded choice.
    * How far can this go?
      NPC: Theoretically forever but who knows?
	* This is an extra question
	  NPC: That should have been added to the previous choice
	* Another question?
	  NPC: Yep, this one too
		* A third level of questions?
		  NPC: Yes, really!
		* Wow
          NPC: IKR
        * Continuation with no response, just fallthrough
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
    NPC: This is another option with an embedded choice.
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
	if (!TestNotNull("Root node should exist", RootNode))
		return false;
	
	TestEqual("Root node type", RootNode->NodeType, ESUDSScriptNodeType::Text);
	TestEqual("Root node speaker", RootNode->Speaker, "Player");
	TestEqual("Root node text", RootNode->Text, "Excuse me?");
	TestEqual("Root node edges", RootNode->Edges.Num(), 1);

	TestFalse("First node edge not jump", RootNode->Edges[0].bIsJump);
	auto NextNode = Importer.GetNode(RootNode->Edges[0].TargetNodeIdx);
	if (!TestNotNull("Next node should exist", NextNode))
		return false;

	TestEqual("Second node type", NextNode->NodeType, ESUDSScriptNodeType::Text);
	TestEqual("Second node speaker", NextNode->Speaker, "NPC");
	TestEqual("Second node text", NextNode->Text, "Well, hello there. This is a test.");
	TestEqual("Second node edges", NextNode->Edges.Num(), 1);

	TestFalse("Second node edge not jump", NextNode->Edges[0].bIsJump);
	NextNode = Importer.GetNode(NextNode->Edges[0].TargetNodeIdx);
	if (!TestNotNull("Third node should exist", NextNode))
		return false;
	TestEqual("Third node type", NextNode->NodeType, ESUDSScriptNodeType::Choice);
	TestEqual("Third node edges", NextNode->Edges.Num(), 2);

	auto Choice1Node = NextNode;
	if (NextNode->Edges.Num() >= 2)
	{
		TestFalse("Choice 1 node edge 0 not jump", Choice1Node->Edges[0].bIsJump);
		TestEqual("Choice 1 node edge 0 text", Choice1Node->Edges[0].Text, "A test?");
		TestEqual("Choice 1 node edge 1 text", Choice1Node->Edges[1].Text, "Another option");
		// Follow choice 1
		NextNode = Importer.GetNode(Choice1Node->Edges[0].TargetNodeIdx);
		if (TestNotNull("Next node should exist", NextNode))
		{
			TestEqual("Choice 1 1st text node type", NextNode->NodeType, ESUDSScriptNodeType::Text);
			TestEqual("Choice 1 1st text node speaker", NextNode->Speaker, "NPC");
			TestEqual("Choice 1 1st text node text", NextNode->Text, "Yes, a test. This is some indented continuation text.");
			TestEqual("Choice 1 1st text node edges", NextNode->Edges.Num(), 1);
			NextNode = Importer.GetNode(NextNode->Edges[0].TargetNodeIdx);
			if (TestNotNull("Next node should exist", NextNode))
			{
				TestEqual("Choice 1 2nd text node type", NextNode->NodeType, ESUDSScriptNodeType::Text);
				TestEqual("Choice 1 2nd text node speaker", NextNode->Speaker, "Player");
				TestEqual("Choice 1 2nd text node text", NextNode->Text, "Oh I see, thank you.");
				TestEqual("Choice 1 2nd text node edges", NextNode->Edges.Num(), 1);
				NextNode = Importer.GetNode(NextNode->Edges[0].TargetNodeIdx);
				if (TestNotNull("Next node should exist", NextNode))
				{
					TestEqual("Choice 1 3rd text node type", NextNode->NodeType, ESUDSScriptNodeType::Text);
					TestEqual("Choice 1 3rd text node speaker", NextNode->Speaker, "NPC");
					TestEqual("Choice 1 3rd text node text", NextNode->Text, "You're welcome.");

					// Should fall through
					TestEqual("Choice 1 3rd text node edges", NextNode->Edges.Num(), 1);
					if (NextNode->Edges.Num() >= 1)
					{
						auto LinkedNode = Importer.GetNode(NextNode->Edges[0].TargetNodeIdx);
						if (TestNotNull("Choice 1 3rd text linked node", LinkedNode))
						{
							TestTrue("Choice 1 3rd text target node", LinkedNode->Text.StartsWith("Well, that's all for now"));
						}
					}
				}
			}
		}


		// Follow choice 2
		NextNode = Importer.GetNode(Choice1Node->Edges[1].TargetNodeIdx);
		if (!TestNotNull("Next node should exist", NextNode))
			return false;
		
		TestEqual("Choice 2 1st text node type", NextNode->NodeType, ESUDSScriptNodeType::Text);
		TestEqual("Choice 2 1st text node speaker", NextNode->Speaker, "NPC");
		TestEqual("Choice 2 1st text node text", NextNode->Text, "This is another option with an embedded choice.");
		TestEqual("Choice 2 1st text node edges", NextNode->Edges.Num(), 1);
		NextNode = Importer.GetNode(NextNode->Edges[0].TargetNodeIdx);
		if (!TestNotNull("Next node should exist", NextNode))
			return false;
		// This is nested choice node
		TestEqual("Choice 2 1st text node type", NextNode->NodeType, ESUDSScriptNodeType::Choice);
		TestEqual("Choice 2 nested choice edges", NextNode->Edges.Num(), 3);

		auto NestedChoiceNode = NextNode;
		if (NestedChoiceNode->Edges.Num() >= 3)
		{
			TestEqual("Nested choice edge text 0", NestedChoiceNode->Edges[0].Text, "How far can this go?");
			
			NextNode = Importer.GetNode(NestedChoiceNode->Edges[0].TargetNodeIdx);
			if (TestNotNull("Next node should exist", NextNode))
			{
				TestEqual("Nested Choice 1st text node type", NextNode->NodeType, ESUDSScriptNodeType::Text);
				TestEqual("Nested Choice 1st text node speaker", NextNode->Speaker, "NPC");
				TestEqual("Nested Choice 1st text node text", NextNode->Text, "Theoretically forever but who knows?");
				if (TestEqual("Nested Choice 1st text node edges", NextNode->Edges.Num(), 1))
				{
					// Should fall through
					auto LinkedNode = Importer.GetNode(NextNode->Edges[0].TargetNodeIdx);
					if (TestNotNull("Nested Choice 1st linked node", LinkedNode))
					{
						TestTrue("Nested Choice 1st text target node", LinkedNode->Text.StartsWith("Well, that's all for now"));
					}
				}
			}

			TestEqual("Nested choice edge text 1", NestedChoiceNode->Edges[1].Text, "This is an extra question");
			NextNode = Importer.GetNode(NestedChoiceNode->Edges[1].TargetNodeIdx);
			if (TestNotNull("Next node should exist", NextNode))
			{
				TestEqual("Nested Choice 2nd text node type", NextNode->NodeType, ESUDSScriptNodeType::Text);
				TestEqual("Nested Choice 2nd text node speaker", NextNode->Speaker, "NPC");
				TestEqual("Nested Choice 2nd text node text", NextNode->Text, "That should have been added to the previous choice");
				TestEqual("Nested Choice 2nd text node edges", NextNode->Edges.Num(), 1);
				if (TestEqual("Nested Choice 2nd text node edges", NextNode->Edges.Num(), 1))
				{
					// Should fall through
					auto LinkedNode = Importer.GetNode(NextNode->Edges[0].TargetNodeIdx);
					if (TestNotNull("Nested Choice 2nd linked node", LinkedNode))
					{
						TestTrue("Nested Choice 2nd text target node", LinkedNode->Text.StartsWith("Well, that's all for now"));
					}
				}
			}
			
			TestEqual("Nested choice edge text 2", NestedChoiceNode->Edges[2].Text, "Another question?");
			NextNode = Importer.GetNode(NestedChoiceNode->Edges[2].TargetNodeIdx);
			if (TestNotNull("Next node should exist", NextNode))
			{
				TestEqual("Nested Choice 3rd text node type", NextNode->NodeType, ESUDSScriptNodeType::Text);
				TestEqual("Nested Choice 3rd text node speaker", NextNode->Speaker, "NPC");
				TestEqual("Nested Choice 3rd text node text", NextNode->Text, "Yep, this one too");
				if (TestEqual("Nested Choice 3rd text node edges", NextNode->Edges.Num(), 1))
				{
					// Double nested
					auto DoubleChoiceNode  = Importer.GetNode(NextNode->Edges[0].TargetNodeIdx);
					if (TestNotNull("Double Nested Choice node", DoubleChoiceNode))
					{
						TestEqual("Double Nested Choice node type", DoubleChoiceNode->NodeType, ESUDSScriptNodeType::Choice);
						if (TestEqual("Double Nested Choice node edges", DoubleChoiceNode->Edges.Num(), 3))
						{
							
							TestEqual("Double Nested choice edge text 0", DoubleChoiceNode->Edges[0].Text, "A third level of questions?");
							NextNode = Importer.GetNode(DoubleChoiceNode->Edges[0].TargetNodeIdx);
							if (TestNotNull("Double Nested Choice node option 0", NextNode))
							{
								TestEqual("Double Nested Choice option 0 text node type", NextNode->NodeType, ESUDSScriptNodeType::Text);
								TestEqual("Double Nested Choice option 0 text node speaker", NextNode->Speaker, "NPC");
								TestEqual("Double Nested Choice option 0 text node text", NextNode->Text, "Yes, really!");
								// Should fall through
								if (TestEqual("Double Nested Choice option 0 text edge", NextNode->Edges.Num(), 1))
								NextNode = Importer.GetNode(NextNode->Edges[0].TargetNodeIdx);
								if (TestNotNull("Double Nested Choice option 0 text linked node", NextNode))
								{
									TestTrue("Double Nested Choice option 0 text linked node", NextNode->Text.StartsWith("Well, that's all for now"));
								}
							}
							
							TestEqual("Double Nested choice edge text 1", DoubleChoiceNode->Edges[1].Text, "Wow");
							NextNode = Importer.GetNode(DoubleChoiceNode->Edges[1].TargetNodeIdx);
							if (TestNotNull("Double Nested Choice node option 1", NextNode))
							{
								TestEqual("Double Nested Choice option 1 text node type", NextNode->NodeType, ESUDSScriptNodeType::Text);
								TestEqual("Double Nested Choice option 1 text node speaker", NextNode->Speaker, "NPC");
								TestEqual("Double Nested Choice option 1 text node text", NextNode->Text, "IKR");
								// Should fall through
								if (TestEqual("Double Nested Choice option 1 text edge", NextNode->Edges.Num(), 1))
									NextNode = Importer.GetNode(NextNode->Edges[0].TargetNodeIdx);
								if (TestNotNull("Double Nested Choice option 0 text linked node", NextNode))
								{
									TestTrue("Double Nested Choice option 0 text linked node", NextNode->Text.StartsWith("Well, that's all for now"));
								}
							}
							
							// Last Should fall through
							TestEqual("Double Nested choice edge text 2", DoubleChoiceNode->Edges[2].Text, "Continuation with no response, just fallthrough");
							auto LinkedNode = Importer.GetNode(DoubleChoiceNode->Edges[2].TargetNodeIdx);
							if (TestNotNull("Nested Choice 3rd linked node", LinkedNode))
							{
								TestTrue("Nested Choice 3rd text target node", LinkedNode->Text.StartsWith("Well, that's all for now"));
							}
						}
						
					}
					



					
				}
			}
			
		}
		
	}		

	return true;
}


PRAGMA_ENABLE_OPTIMIZATION