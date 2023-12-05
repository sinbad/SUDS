#include "SUDSMessageLogger.h"
#include "SUDSScript.h"
#include "Misc/AutomationTest.h"
#include "SUDSScriptImporter.h"
#include "SUDSScriptNode.h"
#include "TestUtils.h"

UE_DISABLE_OPTIMIZATION

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
	Player: This is a level 2 fallthrough 
Player: Well, that's all for now. This should appear for all paths as a fall-through.
	This, in fact, is a multi-line piece of text
	Which is joined to the previous text node with the line breaks
NPC: Bye!
)RAWSUD";

const FString GotoParsingInput = R"RAWSUD(
:start
:alsostart
Player: This is the start
	:choice
	* Go to end
		NPC: How rude, bye then
		[goto end]
	* Nested option
		NPC: Some nested text with <Bounce>formatting</>
		* Go to goodbye
			Player: Gotta go!
			[go to goodbye] 
		* Skip
			[goto secondchoice]
		* This is a <Bounce>mistake</>
			NPC: Oh no
			[goto this_is_an_error]
:secondchoice
NPC: Yep, this one too
	* Go back to choice
		NPC: Okay!
		[goto choice]
	* Return to the start
		NPC: Gotcha
		[goto start]
	* Alternative start, also with no text before
		[goto alsostart]
:goodbye
NPC: Bye!
)RAWSUD";


const FString SetVariableParsingInput = R"RAWSUD(
===
# Set some vars in header
# Text var with an existing localised ID
[set SpeakerName.Player "Protagonist"] @12345@
# Text var no localised ID
[set ValetName "Bob"]
[set SomeFloat 12.5]
[set SomeName `AName`]
[set EmbeddedQuoteString "Hello this has some \"Embedded Quotes\""]
===

Player: Hello
[set SomeInt 99]
NPC: Wotcha
# Test that inserting a set node in between text and choice doesn't break link 
[set SomeGender masculine]
	* Choice 1
		[set SomeBoolean True]
		NPC: Truth
	* Choice 2
		NPC: Surprise
		[set ValetName "Kate"]
		[set SomeGender feminine]
Player: Well
	
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
	FSUDSMessageLogger Logger(false);
	FSUDSScriptImporter Importer;
	TestTrue("Import should succeed", Importer.ImportFromBuffer(GetData(SimpleParsingInput), SimpleParsingInput.Len(), "SimpleParsingInput", &Logger, true));

	// Test the content of the parsing
	auto RootNode = Importer.GetNode(0);
	if (!TestNotNull("Root node should exist", RootNode))
		return false;

	TestEqual("Root node type", RootNode->NodeType, ESUDSParsedNodeType::Text);
	TestEqual("Root node speaker", RootNode->Identifier, "Player");
	TestEqual("Root node text", RootNode->Text, "Excuse me?");
	TestEqual("Root node path", RootNode->ChoicePath, "/");
	TestEqual("Root node edges", RootNode->Edges.Num(), 1);

	auto NextNode = Importer.GetNode(RootNode->Edges[0].TargetNodeIdx);
	if (!TestNotNull("Next node should exist", NextNode))
		return false;

	TestEqual("Second node type", NextNode->NodeType, ESUDSParsedNodeType::Text);
	TestEqual("Second node speaker", NextNode->Identifier, "NPC");
	TestEqual("Second node text", NextNode->Text, "Well, hello there. This is a test.");
	TestEqual("Second node path", RootNode->ChoicePath, "/");
	TestEqual("Second node edges", NextNode->Edges.Num(), 1);

	NextNode = Importer.GetNode(NextNode->Edges[0].TargetNodeIdx);
	if (!TestNotNull("Third node should exist", NextNode))
		return false;
	TestEqual("Third node type", NextNode->NodeType, ESUDSParsedNodeType::Choice);
	// Choice itself is still at root, only the edges (individual choices) introduce new path levels
	TestEqual("Third node path", NextNode->ChoicePath, "/");
	TestEqual("Third node edges", NextNode->Edges.Num(), 2);

	auto Choice1Node = NextNode;
	const FSUDSParsedNode* FallthroughNode = nullptr;
	if (NextNode->Edges.Num() >= 2)
	{
		TestEqual("Choice 1 node edge 0 text", Choice1Node->Edges[0].Text, "A test?");
		TestEqual("Choice 1 node edge 1 text", Choice1Node->Edges[1].Text, "Another option");
		// Follow choice 1
		NextNode = Importer.GetNode(Choice1Node->Edges[0].TargetNodeIdx);
		if (TestNotNull("Next node should exist", NextNode))
		{
			TestEqual("Choice 1 1st text node type", NextNode->NodeType, ESUDSParsedNodeType::Text);
			TestEqual("Choice 1 1st text node speaker", NextNode->Identifier, "NPC");
			TestEqual("Choice 1 1st text node text", NextNode->Text, "Yes, a test. This is some indented continuation text.");
			TestEqual("Choice 1 1st text node path", NextNode->ChoicePath, "/C001/");
			TestEqual("Choice 1 1st text node edges", NextNode->Edges.Num(), 1);
			NextNode = Importer.GetNode(NextNode->Edges[0].TargetNodeIdx);
			if (TestNotNull("Next node should exist", NextNode))
			{
				TestEqual("Choice 1 2nd text node type", NextNode->NodeType, ESUDSParsedNodeType::Text);
				TestEqual("Choice 1 2nd text node speaker", NextNode->Identifier, "Player");
				TestEqual("Choice 1 2nd text node text", NextNode->Text, "Oh I see, thank you.");
				TestEqual("Choice 1 2nd text node path", NextNode->ChoicePath, "/C001/");
				TestEqual("Choice 1 2nd text node edges", NextNode->Edges.Num(), 1);
				NextNode = Importer.GetNode(NextNode->Edges[0].TargetNodeIdx);
				if (TestNotNull("Next node should exist", NextNode))
				{
					TestEqual("Choice 1 3rd text node type", NextNode->NodeType, ESUDSParsedNodeType::Text);
					TestEqual("Choice 1 3rd text node speaker", NextNode->Identifier, "NPC");
					TestEqual("Choice 1 3rd text node text", NextNode->Text, "You're welcome.");
					TestEqual("Choice 1 3rd text node path", NextNode->ChoicePath, "/C001/");

					// Should fall through, all the way to the end and not to "level 2 fallthrough" since that's deeper level
					TestEqual("Choice 1 3rd text node edges", NextNode->Edges.Num(), 1);
					if (NextNode->Edges.Num() >= 1)
					{
						auto LinkedNode = Importer.GetNode(NextNode->Edges[0].TargetNodeIdx);
						if (TestNotNull("Choice 1 3rd text linked node", LinkedNode))
						{
							TestTrue("Choice 1 3rd text target node", LinkedNode->Text.StartsWith("Well, that's all for now"));
							FallthroughNode = LinkedNode;
						}
					}
				}
			}
		}


		// Follow choice 2
		NextNode = Importer.GetNode(Choice1Node->Edges[1].TargetNodeIdx);
		if (!TestNotNull("Next node should exist", NextNode))
			return false;
		
		TestEqual("Choice 2 1st text node type", NextNode->NodeType, ESUDSParsedNodeType::Text);
		TestEqual("Choice 2 1st text node speaker", NextNode->Identifier, "NPC");
		TestEqual("Choice 2 1st text node text", NextNode->Text, "This is another option with an embedded choice.");
		TestEqual("Choice 2 2nd text node path", NextNode->ChoicePath, "/C002/");
		TestEqual("Choice 2 1st text node edges", NextNode->Edges.Num(), 1);
		NextNode = Importer.GetNode(NextNode->Edges[0].TargetNodeIdx);
		if (!TestNotNull("Next node should exist", NextNode))
			return false;
		// This is nested choice node
		TestEqual("Choice 2 1st text node type", NextNode->NodeType, ESUDSParsedNodeType::Choice);
		TestEqual("Choice 2 nested choice edges", NextNode->Edges.Num(), 3);

		auto NestedChoiceNode = NextNode;
		if (NestedChoiceNode->Edges.Num() >= 3)
		{
			TestEqual("Nested choice edge text 0", NestedChoiceNode->Edges[0].Text, "How far can this go?");
			
			NextNode = Importer.GetNode(NestedChoiceNode->Edges[0].TargetNodeIdx);
			if (TestNotNull("Next node should exist", NextNode))
			{
				TestEqual("Nested Choice 1st text node type", NextNode->NodeType, ESUDSParsedNodeType::Text);
				TestEqual("Nested Choice 1st text node speaker", NextNode->Identifier, "NPC");
				TestEqual("Nested Choice 1st text node text", NextNode->Text, "Theoretically forever but who knows?");
				// Choice edges are assigned unique numbers in ascending order, but nested
				// This helps with fallthrough
				TestEqual("Nested Choice 1st text node path", NextNode->ChoicePath, "/C002/C003/");

				if (TestEqual("Nested Choice 1st text node edges", NextNode->Edges.Num(), 1))
				{
					// Should fall through
					auto LinkedNode = Importer.GetNode(NextNode->Edges[0].TargetNodeIdx);
					if (TestNotNull("Nested Choice 1st linked node", LinkedNode))
					{
						TestEqual("Nested Choice 1st text target node", LinkedNode->Text, "This is a level 2 fallthrough");
					}
				}
			}

			TestEqual("Nested choice edge text 1", NestedChoiceNode->Edges[1].Text, "This is an extra question");
			NextNode = Importer.GetNode(NestedChoiceNode->Edges[1].TargetNodeIdx);
			if (TestNotNull("Next node should exist", NextNode))
			{
				TestEqual("Nested Choice 2nd text node type", NextNode->NodeType, ESUDSParsedNodeType::Text);
				TestEqual("Nested Choice 2nd text node speaker", NextNode->Identifier, "NPC");
				TestEqual("Nested Choice 2nd text node text", NextNode->Text, "That should have been added to the previous choice");
				TestEqual("Nested Choice 2nd text node path", NextNode->ChoicePath, "/C002/C004/");
				TestEqual("Nested Choice 2nd text node edges", NextNode->Edges.Num(), 1);
				if (TestEqual("Nested Choice 2nd text node edges", NextNode->Edges.Num(), 1))
				{
					// Should fall through
					auto LinkedNode = Importer.GetNode(NextNode->Edges[0].TargetNodeIdx);
					if (TestNotNull("Nested Choice 2nd linked node", LinkedNode))
					{
						TestEqual("Nested Choice 2nd text target node", LinkedNode->Text, "This is a level 2 fallthrough");
					}
				}
			}
			
			TestEqual("Nested choice edge text 2", NestedChoiceNode->Edges[2].Text, "Another question?");
			NextNode = Importer.GetNode(NestedChoiceNode->Edges[2].TargetNodeIdx);
			if (TestNotNull("Next node should exist", NextNode))
			{
				TestEqual("Nested Choice 3rd text node type", NextNode->NodeType, ESUDSParsedNodeType::Text);
				TestEqual("Nested Choice 3rd text node speaker", NextNode->Identifier, "NPC");
				TestEqual("Nested Choice 3rd text node text", NextNode->Text, "Yep, this one too");
				TestEqual("Nested Choice 3rd text node path", NextNode->ChoicePath, "/C002/C005/");
				if (TestEqual("Nested Choice 3rd text node edges", NextNode->Edges.Num(), 1))
				{
					// Double nested
					auto DoubleChoiceNode  = Importer.GetNode(NextNode->Edges[0].TargetNodeIdx);
					if (TestNotNull("Double Nested Choice node", DoubleChoiceNode))
					{
						TestEqual("Double Nested Choice node type", DoubleChoiceNode->NodeType, ESUDSParsedNodeType::Choice);
						if (TestEqual("Double Nested Choice node edges", DoubleChoiceNode->Edges.Num(), 3))
						{
							
							TestEqual("Double Nested choice edge text 0", DoubleChoiceNode->Edges[0].Text, "A third level of questions?");
							NextNode = Importer.GetNode(DoubleChoiceNode->Edges[0].TargetNodeIdx);
							if (TestNotNull("Double Nested Choice node option 0", NextNode))
							{
								TestEqual("Double Nested Choice option 0 text node type", NextNode->NodeType, ESUDSParsedNodeType::Text);
								TestEqual("Double Nested Choice option 0 text node speaker", NextNode->Identifier, "NPC");
								TestEqual("Double Nested Choice option 0 text node text", NextNode->Text, "Yes, really!");
								// Should fall through
								if (TestEqual("Double Nested Choice option 0 text edge", NextNode->Edges.Num(), 1))
								NextNode = Importer.GetNode(NextNode->Edges[0].TargetNodeIdx);
								if (TestNotNull("Double Nested Choice option 0 text linked node", NextNode))
								{
									TestEqual("Double Nested Choice option 0 text linked node", NextNode->Text, "This is a level 2 fallthrough");
								}
							}
							
							TestEqual("Double Nested choice edge text 1", DoubleChoiceNode->Edges[1].Text, "Wow");
							NextNode = Importer.GetNode(DoubleChoiceNode->Edges[1].TargetNodeIdx);
							if (TestNotNull("Double Nested Choice node option 1", NextNode))
							{
								TestEqual("Double Nested Choice option 1 text node type", NextNode->NodeType, ESUDSParsedNodeType::Text);
								TestEqual("Double Nested Choice option 1 text node speaker", NextNode->Identifier, "NPC");
								TestEqual("Double Nested Choice option 1 text node text", NextNode->Text, "IKR");
								// Should fall through
								if (TestEqual("Double Nested Choice option 1 text edge", NextNode->Edges.Num(), 1))
									NextNode = Importer.GetNode(NextNode->Edges[0].TargetNodeIdx);
								if (TestNotNull("Double Nested Choice option 0 text linked node", NextNode))
								{
									TestEqual("Double Nested Choice option 0 text linked node", NextNode->Text, "This is a level 2 fallthrough");
								}
							}
							
							// Last Should fall through
							TestEqual("Double Nested choice edge text 2", DoubleChoiceNode->Edges[2].Text, "Continuation with no response, just fallthrough");
							auto LinkedNode = Importer.GetNode(DoubleChoiceNode->Edges[2].TargetNodeIdx);
							if (TestNotNull("Nested Choice 3rd linked node", LinkedNode))
							{
								TestEqual("Nested Choice 3rd text target node", LinkedNode->Text, "This is a level 2 fallthrough");
							}
						}
						
					}
					
				}
			}
			
		}

		if (TestNotNull("Should have found fallthrough node", FallthroughNode))
		{
			// Test the final fallthrough
			TestEqual("Fallthrough node type", FallthroughNode->NodeType, ESUDSParsedNodeType::Text);
			TestEqual("Fallthrough node speaker", FallthroughNode->Identifier, "Player");
			TestEqual("Fallthrough node text", FallthroughNode->Text, "Well, that's all for now. This should appear for all paths as a fall-through.\nThis, in fact, is a multi-line piece of text\nWhich is joined to the previous text node with the line breaks");
			if (TestEqual("Fallthrough node edge count", FallthroughNode->Edges.Num(), 1))
			{
				NextNode = Importer.GetNode(FallthroughNode->Edges[0].TargetNodeIdx);
				if (TestNotNull("Fallthrough node next node not null", NextNode))
				{
					TestEqual("Fallthrough node 2 type", NextNode->NodeType, ESUDSParsedNodeType::Text);
					TestEqual("Fallthrough node 2 speaker", NextNode->Identifier, "NPC");
					TestEqual("Fallthrough node 2 text", NextNode->Text, "Bye!");
					// Should have no further edges since is at end
					TestEqual("Fallthrough node 2 edge count", NextNode->Edges.Num(), 0);
				}
			}
			
			
		}
		
	}		

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestGotoParsing,
								 "SUDSTest.TestGotoParsing",
								 EAutomationTestFlags::EditorContext |
								 EAutomationTestFlags::ClientContext |
								 EAutomationTestFlags::ProductFilter)


bool FTestGotoParsing::RunTest(const FString& Parameters)
{
	FSUDSMessageLogger Logger(false);
	FSUDSScriptImporter Importer;
	TestTrue("Import should succeed", Importer.ImportFromBuffer(GetData(GotoParsingInput), GotoParsingInput.Len(), "GotoParsingInput", &Logger, true));

	// Test the content of the parsing
	auto RootNode = Importer.GetNode(0);
	if (!TestNotNull("Root node should exist", RootNode))
		return false;

	TestEqual("Root node type", RootNode->NodeType, ESUDSParsedNodeType::Text);
	TestEqual("Root node speaker", RootNode->Identifier, "Player");
	TestEqual("Root node text", RootNode->Text, "This is the start");
	TestEqual("Root node edges", RootNode->Edges.Num(), 1);

	auto NextNode = Importer.GetNode(RootNode->Edges[0].TargetNodeIdx);
	if (!TestNotNull("Next node should exist", NextNode))
		return false;

	TestEqual("Choice node type", NextNode->NodeType, ESUDSParsedNodeType::Choice);
	if (TestEqual("Choice node edges", NextNode->Edges.Num(), 2))
	{
		auto ChoiceNode = NextNode;
		TestEqual("Choice 1 text", ChoiceNode->Edges[0].Text, "Go to end");
		NextNode = Importer.GetNode(ChoiceNode->Edges[0].TargetNodeIdx);
		if (TestNotNull("Next node should not be null", NextNode))
		{
			TestEqual("Goto End node text type", NextNode->NodeType, ESUDSParsedNodeType::Text);
			TestEqual("Goto End node text speaker", NextNode->Identifier, "NPC");
			TestEqual("Goto End node text text", NextNode->Text, "How rude, bye then");
			if (TestEqual("Goto End node text edges", NextNode->Edges.Num(), 1))
			{
				NextNode = Importer.GetNode(NextNode->Edges[0].TargetNodeIdx);
				if (TestNotNull("Goto node should not be null", NextNode))
				{
					TestEqual("Goto End node type", NextNode->NodeType, ESUDSParsedNodeType::Goto);
					TestEqual("Goto End node label", NextNode->Identifier, FSUDSScriptImporter::EndGotoLabel);
				}
				
			}
			
		}
		TestEqual("Choice 2 text", ChoiceNode->Edges[1].Text, "Nested option");
		NextNode = Importer.GetNode(ChoiceNode->Edges[1].TargetNodeIdx);
		if (TestNotNull("Next node should not be null", NextNode))
		{
			TestEqual("Choice 2 node text type", NextNode->NodeType, ESUDSParsedNodeType::Text);
			TestEqual("Choice 2 node text speaker", NextNode->Identifier, "NPC");
			TestEqual("Choice 2 node text text", NextNode->Text, "Some nested text with <Bounce>formatting</>");
			if (TestEqual("Choice 2 node text edges", NextNode->Edges.Num(), 1))
			{
				NextNode = Importer.GetNode(NextNode->Edges[0].TargetNodeIdx);
				if (TestNotNull("Nested choice node should not be null", NextNode))
				{
					TestEqual("Goto End node type", NextNode->NodeType, ESUDSParsedNodeType::Choice);
					auto NestedChoice = NextNode;
					if (TestEqual("Nested Choice node edges", NextNode->Edges.Num(), 3))
					{
						TestEqual("Nested choice 0 text", NestedChoice->Edges[0].Text, "Go to goodbye");
						NextNode = Importer.GetNode(NestedChoice->Edges[0].TargetNodeIdx);
						if (TestNotNull("Should not be null", NextNode))
						{
							TestEqual("Nested choice 0 text type", NextNode->NodeType, ESUDSParsedNodeType::Text);
							TestEqual("Nested choice 0 text speaker", NextNode->Identifier, "Player");
							TestEqual("Nested choice 0 text text", NextNode->Text, "Gotta go!");

							if (TestEqual("Nested Choice textnode edges", NextNode->Edges.Num(), 1))
							{
								NextNode = Importer.GetNode(NextNode->Edges[0].TargetNodeIdx);
								if (TestNotNull("Should not be null", NextNode))
								{
									// This is the goto
									TestEqual("Nested choice 0 goto type", NextNode->NodeType, ESUDSParsedNodeType::Goto);
									TestEqual("Nested choice 0 goto label", NextNode->Identifier, "goodbye");
									const int DestIdx = Importer.GetGotoTargetNodeIndex(NextNode->Identifier);
									TestNotEqual("Label should be valid", DestIdx, -1);
									NextNode = Importer.GetNode(DestIdx);
									if (TestNotNull("Goto dest node", NextNode))
									{
										TestEqual("Goto dest text type", NextNode->NodeType, ESUDSParsedNodeType::Text);
										TestEqual("Goto dest text speaker", NextNode->Identifier, "NPC");
										TestEqual("Goto dest text text", NextNode->Text, "Bye!");
									}
								}
							}
						}
						TestEqual("Nested choice 1 text", NestedChoice->Edges[1].Text, "Skip");
						NextNode = Importer.GetNode(NestedChoice->Edges[1].TargetNodeIdx);
						if (TestNotNull("Should not be null", NextNode))
						{
							// This one goes straight to goto
							TestEqual("Nested choice 1 goto type", NextNode->NodeType, ESUDSParsedNodeType::Goto);
							TestEqual("Nested choice 1 goto label", NextNode->Identifier, "secondchoice");

							const int DestIdx = Importer.GetGotoTargetNodeIndex(NextNode->Identifier);
							TestNotEqual("Label should be valid", DestIdx, -1);
							NextNode = Importer.GetNode(DestIdx);
							if (TestNotNull("Goto dest node", NextNode))
							{
								TestEqual("Goto dest text type", NextNode->NodeType, ESUDSParsedNodeType::Text);
								TestEqual("Goto dest text speaker", NextNode->Identifier, "NPC");
								TestEqual("Goto dest text text", NextNode->Text, "Yep, this one too");
										
							}
						}
						TestEqual("Nested choice 2 text", NestedChoice->Edges[2].Text, "This is a <Bounce>mistake</>");
						NextNode = Importer.GetNode(NestedChoice->Edges[2].TargetNodeIdx);
						if (TestNotNull("Should not be null", NextNode))
						{
							TestEqual("Nested choice 2 text type", NextNode->NodeType, ESUDSParsedNodeType::Text);
							TestEqual("Nested choice 2 text speaker", NextNode->Identifier, "NPC");
							TestEqual("Nested choice 2 text text", NextNode->Text, "Oh no");

							if (TestEqual("Nested Choice text node edges", NextNode->Edges.Num(), 1))
							{
								NextNode = Importer.GetNode(NextNode->Edges[0].TargetNodeIdx);
								if (TestNotNull("Should not be null", NextNode))
								{
									// This is the goto
									TestEqual("Nested choice 0 goto type", NextNode->NodeType, ESUDSParsedNodeType::Goto);
									TestEqual("Nested choice 0 goto label", NextNode->Identifier, "this_is_an_error");
									TestEqual("Label should go nowhere", Importer.GetGotoTargetNodeIndex(NextNode->Identifier), -1);
								}
							}
							
						}
						
					}
				}
				
			}
			
		}

		// Pick up the latter part (only reachable by goto)
		NextNode = Importer.GetNode(Importer.GetGotoTargetNodeIndex("secondchoice"));
		if (TestNotNull("End choice node should not be null", NextNode))
		{
			TestEqual("End choice node text type", NextNode->NodeType, ESUDSParsedNodeType::Text);
			TestEqual("End choice node text speaker", NextNode->Identifier, "NPC");
			TestEqual("End choice node text text", NextNode->Text, "Yep, this one too");
			if (TestEqual("End choice node text edges", NextNode->Edges.Num(), 1))
			{
				NextNode = Importer.GetNode(NextNode->Edges[0].TargetNodeIdx);
				if (TestNotNull("", NextNode))
				{
					TestEqual("End choice node text type", NextNode->NodeType, ESUDSParsedNodeType::Choice);
								
					if (TestEqual("End choice node edges", NextNode->Edges.Num(), 3))
					{
						auto EndChoiceNode = NextNode;
						TestEqual("End choice 0 text", EndChoiceNode->Edges[0].Text, "Go back to choice");
						NextNode = Importer.GetNode(EndChoiceNode->Edges[0].TargetNodeIdx);
						if (TestNotNull("", NextNode))
						{
							TestEqual("Node text type", NextNode->NodeType, ESUDSParsedNodeType::Text);
							TestEqual("Node text speaker", NextNode->Identifier, "NPC");
							TestEqual("Node text text", NextNode->Text, "Okay!");
							if (TestEqual("Next node edges", NextNode->Edges.Num(), 1))
							{
								NextNode = Importer.GetNode(NextNode->Edges[0].TargetNodeIdx);
								if (TestNotNull("", NextNode))
								{
									TestEqual("Should be goto node", NextNode->NodeType, ESUDSParsedNodeType::Goto);
									TestEqual("Goto label", NextNode->Identifier, "choice");
									
									// This should lead back to a choice node, ie letting the previous text node use the same choices
									// without repeating the text (loop with context)
									const int DestIdx = Importer.GetGotoTargetNodeIndex(NextNode->Identifier);
									TestNotEqual("Label should be valid", DestIdx, -1);
									NextNode = Importer.GetNode(DestIdx);
									if (TestNotNull("Goto dest node", NextNode))
									{
										TestEqual("Goto dest choice type", NextNode->NodeType, ESUDSParsedNodeType::Choice);
										// Just make sure it's the right choice node
										if (TestEqual("Goto dest choice edges", NextNode->Edges.Num(), 2))
										{
											TestEqual("Goto dest choice check edge", NextNode->Edges[0].Text, "Go to end");
										}
									}
									
								}
							}

							NextNode = Importer.GetNode(EndChoiceNode->Edges[1].TargetNodeIdx);
							if (TestNotNull("", NextNode))
							{
								TestEqual("Node text type", NextNode->NodeType, ESUDSParsedNodeType::Text);
								TestEqual("Node text speaker", NextNode->Identifier, "NPC");
								TestEqual("Node text text", NextNode->Text, "Gotcha");
								if (TestEqual("Next node edges", NextNode->Edges.Num(), 1))
								{
									NextNode = Importer.GetNode(NextNode->Edges[0].TargetNodeIdx);
									if (TestNotNull("", NextNode))
									{
										TestEqual("Should be goto node", NextNode->NodeType, ESUDSParsedNodeType::Goto);
										TestEqual("Goto label", NextNode->Identifier, "start");
									
										// This should lead back to a choice node, ie letting the previous text node use the same choices
										// without repeating the text (loop with context)
										const int DestIdx = Importer.GetGotoTargetNodeIndex(NextNode->Identifier);
										TestNotEqual("Label should be valid", DestIdx, -1);
										NextNode = Importer.GetNode(DestIdx);
										if (TestNotNull("Goto dest node", NextNode))
										{
											// Just make sure it's the right node
											TestEqual("Goto dest text type", NextNode->NodeType, ESUDSParsedNodeType::Text);
											TestEqual("Node text speaker", NextNode->Identifier, "Player");
											TestEqual("Node text text", NextNode->Text, "This is the start");
										}
									
									}
								}
							}
							NextNode = Importer.GetNode(EndChoiceNode->Edges[2].TargetNodeIdx);
							if (TestNotNull("", NextNode))
							{
								// Straight to goto
								TestEqual("Should be goto node", NextNode->NodeType, ESUDSParsedNodeType::Goto);
								TestEqual("Goto label", NextNode->Identifier, "alsostart");
							
								// This should lead back to a choice node, ie letting the previous text node use the same choices
								// without repeating the text (loop with context)
								const int DestIdx = Importer.GetGotoTargetNodeIndex(NextNode->Identifier);
								TestNotEqual("Label should be valid", DestIdx, -1);
								NextNode = Importer.GetNode(DestIdx);
								if (TestNotNull("Goto dest node", NextNode))
								{
									// Just make sure it's the right node
									TestEqual("Goto dest text type", NextNode->NodeType, ESUDSParsedNodeType::Text);
									TestEqual("Node text speaker", NextNode->Identifier, "Player");
									TestEqual("Node text text", NextNode->Text, "This is the start");
								}
							}
						}
					}
				}
			}

			
			
		}

		// Test that start & autostart point to the same place
		const int StartNodeIdx = Importer.GetGotoTargetNodeIndex("start");
		const auto StartNode = Importer.GetNode(StartNodeIdx);
		if (TestNotNull("Second choice node should not be null", StartNode))
		{
			TestEqual("Start node text type", StartNode->NodeType, ESUDSParsedNodeType::Text);
			TestEqual("Start node text speaker", StartNode->Identifier, "Player");
			TestEqual("Start node text text", StartNode->Text, "This is the start");
		}
		const int AlsoStartNodeIdx = Importer.GetGotoTargetNodeIndex("alsostart");
		TestEqual("start and alsostart should reference the same node", AlsoStartNodeIdx, StartNodeIdx);
		
	}
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestSetVariableParsing,
								 "SUDSTest.TestSetVariableParsing",
								 EAutomationTestFlags::EditorContext |
								 EAutomationTestFlags::ClientContext |
								 EAutomationTestFlags::ProductFilter)


bool FTestSetVariableParsing::RunTest(const FString& Parameters)
{
	FSUDSMessageLogger Logger(false);
	FSUDSScriptImporter Importer;
	TestTrue("Import should succeed", Importer.ImportFromBuffer(GetData(SetVariableParsingInput), SetVariableParsingInput.Len(), "SetVariableParsingInput", &Logger, true));
	// Test the content of the parsing

	// Header nodes first
	auto NextNode = Importer.GetHeaderNode(0);
	TestParsedSetLiteral(this, "Header node 1", NextNode, "SpeakerName.Player", "Protagonist");
	TestGetParsedNextNode(this, "Header node 1 next", NextNode, Importer, true, &NextNode);
	TestParsedSetLiteral(this, "Header node 2", NextNode, "ValetName", "Bob");
	TestGetParsedNextNode(this, "Header node 2 next", NextNode, Importer, true, &NextNode);
	TestParsedSetLiteral(this, "Header node 3", NextNode, "SomeFloat", 12.5f);
	TestGetParsedNextNode(this, "Header node 3 next", NextNode, Importer, true, &NextNode);
	TestParsedSetLiteral(this, "Header node 4", NextNode, "SomeName", FName("AName"));
	TestGetParsedNextNode(this, "Header node 4 next", NextNode, Importer, true, &NextNode);
	TestParsedSetLiteral(this, "Header node 5", NextNode, "EmbeddedQuoteString", "Hello this has some \"Embedded Quotes\"");

	// Now body nodes
	NextNode = Importer.GetNode(0);
	TestParsedText(this, "Root node", NextNode, "Player", "Hello");
	TestGetParsedNextNode(this, "Node 1 next", NextNode, Importer, false, &NextNode);
	TestParsedSetLiteral(this, "Node 2", NextNode, "SomeInt", 99);
	TestGetParsedNextNode(this, "Node 2 next", NextNode, Importer, false, &NextNode);
	TestParsedText(this, "Node 3 text", NextNode, "NPC", "Wotcha");
	TestGetParsedNextNode(this, "Node 3 next", NextNode, Importer, false, &NextNode);
	TestParsedSetLiteral(this, "Node 4", NextNode, "SomeGender", ETextGender::Masculine);
	TestGetParsedNextNode(this, "Node 4 next", NextNode, Importer, false, &NextNode);
	TestParsedChoice(this, "Node 5 choice", NextNode, 2);
	auto ChoiceNode = NextNode;
	TestParsedChoiceEdge(this, "Choice 1 edge", ChoiceNode, 0, "Choice 1", Importer, &NextNode);
	TestParsedSetLiteral(this, "Choice 1 set", NextNode, "SomeBoolean", true);
	TestGetParsedNextNode(this, "Choice 1 next", NextNode, Importer, false, &NextNode);
	TestParsedText(this, "Choice 1 text", NextNode, "NPC", "Truth");

	TestParsedChoiceEdge(this, "Choice 2 edge", ChoiceNode, 1, "Choice 2", Importer, &NextNode);
	TestParsedText(this, "Choice 2 text", NextNode, "NPC", "Surprise");
	TestGetParsedNextNode(this, "Choice 2 next", NextNode, Importer, false, &NextNode);
	TestParsedSetLiteral(this, "Choice 2 set", NextNode, "ValetName", "Kate");
	TestGetParsedNextNode(this, "Choice 2 next 2", NextNode, Importer, false, &NextNode);
	TestParsedSetLiteral(this, "Choice 2 set 2", NextNode, "SomeGender", ETextGender::Feminine);
	
	return true;
}




IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestConversionToRuntime,
								 "SUDSTest.TestConversionToRuntime",
								 EAutomationTestFlags::EditorContext |
								 EAutomationTestFlags::ClientContext |
								 EAutomationTestFlags::ProductFilter)


bool FTestConversionToRuntime::RunTest(const FString& Parameters)
{
	FSUDSMessageLogger Logger(false);
	FSUDSScriptImporter Importer;
	TestTrue("Import should succeed", Importer.ImportFromBuffer(GetData(GotoParsingInput), GotoParsingInput.Len(), "GotoParsingInput", &Logger, true));

	auto Asset = NewObject<USUDSScript>(GetTransientPackage(), "Test");
	const ScopedStringTableHolder StringTableHolder;
	Importer.PopulateAsset(Asset, StringTableHolder.StringTable);

	auto StartNode = Asset->GetFirstNode();
	if (!TestNotNull("Start node should be true", StartNode))
	{
		return false;
	}

	TestTextNode(this, "Start node", StartNode, "Player", "This is the start");

	auto NextNode = StartNode;
	if (!TestEqual("Start node edges", NextNode->GetEdgeCount(), 1))
	{
		return false;
	}
	
	auto pEdge = NextNode->GetEdge(0);
	if (!TestNotNull("Start node edge", pEdge))
	{
		return false;
	}
	
	NextNode = pEdge->GetTargetNode().Get();
	if (TestChoiceNode(this, "Start node next", NextNode, 2))
	{
		auto ChoiceNode = NextNode;
		if (TestChoiceEdge(this, "Choice 1 text", ChoiceNode, 0, "Go to end", &NextNode))
		{
			TestTextNode(this, "Goto End node text type", NextNode, "NPC", "How rude, bye then");
			// This is a goto end so should just have one edge
			auto GotoEdge = NextNode->GetEdge(0);
			if (TestNotNull("Goto end node", GotoEdge))
			{
				TestFalse("Goto end should be null", GotoEdge->GetTargetNode().IsValid());
			}
		}
		
		if (TestChoiceEdge(this, "Choice 2 text", ChoiceNode, 1, "Nested option", &NextNode))
		{
			TestTextNode(this, "Choice 2 node text type", NextNode, "NPC", "Some nested text with <Bounce>formatting</>");
			if (TestEdge(this, "Choice 2 node text edges", NextNode, 0, &NextNode))
			{
				if (TestChoiceNode(this, "Goto End node type", NextNode, 3))
				{
					auto NestedChoice = NextNode;
					if (TestChoiceEdge(this, "Nested choice 0 text", NestedChoice, 0, "Go to goodbye", &NextNode))
					{
						TestTextNode(this, "Nested choice 0 text type", NextNode, "Player", "Gotta go!");
						if (TestEdge(this, "Nested Choice textnode edges", NextNode, 0, &NextNode))
						{
							// This was the goto which should have led to goodbye
							TestTextNode(this, "Check goto goodbye", NextNode, "NPC", "Bye!");
						}
					}

					if (TestChoiceEdge(this, "Nested choice 1 text", NestedChoice, 1, "Skip", &NextNode))
					{
						// This will go directly to secondchoice node
						TestTextNode(this, "Nested choice 1 goto", NextNode, "NPC", "Yep, this one too");
					}

					if (TestChoiceEdge(this, "Nested choice 2 text", NestedChoice, 2, "This is a <Bounce>mistake</>", &NextNode))
					{
						// there's a text node then a failed goto (nowhere)
						TestTextNode(this, "Nested choice 2 node", NextNode, "NPC", "Oh no");
						auto GotoEdge = NextNode->GetEdge(0);
						if (TestNotNull("Nested choice 2 node edge", GotoEdge))
						{
							TestFalse("Nested choice 2 node goes nowhere", GotoEdge->GetTargetNode().IsValid());
						}
						
					}
				}
			}
		}

		// Pick up the latter part (only reachable by goto)
		NextNode = Asset->GetNodeByLabel("secondchoice");
		if (TestTextNode(this, "secondchoice node", NextNode, "NPC", "Yep, this one too"))
		{
			if (TestEdge(this, "secondchoice node text edges", NextNode, 0, &NextNode))
			{
				if (TestChoiceNode(this, "secondchoice next choice", NextNode, 3))
				{
					auto EndChoiceNode = NextNode;
					if (TestChoiceEdge(this, "End choice 0 edge", EndChoiceNode, 0, "Go back to choice", &NextNode))
					{
						TestTextNode(this, "Node text", NextNode, "NPC", "Okay!");
						if (TestEdge(this, "Next node edges", NextNode, 0, &NextNode))
						{
							// Should go back to :choice node
							TestChoiceNode(this, "Check choice goto", NextNode, 2);
							TestChoiceEdge(this, "Check choice goto", NextNode, 0, "Go to end", &NextNode);
						}
					}
					if (TestChoiceEdge(this, "End choice 1 edge", EndChoiceNode, 1, "Return to the start", &NextNode))
					{
						TestTextNode(this, "Node text", NextNode, "NPC", "Gotcha");
						if (TestEdge(this, "Next node edges", NextNode, 0, &NextNode))
						{
							// Should go back to :start
							TestTextNode(this, "Check goto text", NextNode, "Player", "This is the start");
						}
					}
					if (TestChoiceEdge(this, "End choice 2 edge", EndChoiceNode, 2, "Alternative start, also with no text before", &NextNode))
					{
						// Should go directly back to :alsostart, same as :start
						TestTextNode(this, "Check goto text", NextNode, "Player", "This is the start");
					}
				}
			}
		}
	}


	

	// check goto labels
	auto GotoNode = Asset->GetNodeByLabel("start");
	TestTextNode(this, "Goto Start", GotoNode, "Player", "This is the start");
	GotoNode = Asset->GetNodeByLabel("alsostart");
	TestTextNode(this, "Goto alsostart", GotoNode, "Player", "This is the start");
	GotoNode = Asset->GetNodeByLabel("choice");
	TestChoiceNode(this, "Goto choice", GotoNode, 2);
	GotoNode = Asset->GetNodeByLabel("secondchoice");
	TestTextNode(this, "Goto secondchoice", GotoNode, "NPC", "Yep, this one too");
	GotoNode = Asset->GetNodeByLabel("goodbye");
	TestTextNode(this, "Goto goodbye", GotoNode, "NPC", "Bye!");

	// Test speakers
	TestEqual("Num speakers", Asset->GetSpeakers().Num(), 2);
	TestTrue("Speaker 1", Asset->GetSpeakers().Contains("Player"));
	TestTrue("Speaker 2", Asset->GetSpeakers().Contains("NPC"));

	return true;
}

const FString PartiallyLocalisedInput = R"RAWSUD(
Vagabond: Well met, fellow!
  * Er, hi?    
	Vagabond: Verily, 'tis wondrous to see such a fine fellow on the road this morn!
	[goto FriendlyChat]
  * Jog on, mate    @0001@
	Vagabond: Well, really! Good day then sir!    @0002@
	[goto end]

:FriendlyChat
Vagabond: Mayhaps we could travel together a while, and share a tale or two?    @0007@
What do you say?
)RAWSUD";

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestPartiallyLocalised,
								 "SUDSTest.TestPartiallyLocalised",
								 EAutomationTestFlags::EditorContext |
								 EAutomationTestFlags::ClientContext |
								 EAutomationTestFlags::ProductFilter)


bool FTestPartiallyLocalised::RunTest(const FString& Parameters)
{
	FSUDSMessageLogger Logger(false);
	FSUDSScriptImporter Importer;
	TestTrue("Import should succeed", Importer.ImportFromBuffer(GetData(PartiallyLocalisedInput), PartiallyLocalisedInput.Len(), "PartiallyLocalisedInput", &Logger, true));

	// Test the content of the parsing
	auto NextNode = Importer.GetNode(0);

	TestParsedText(this, "Start node", NextNode, "Vagabond", "Well met, fellow!");
	// Test that we generated unique textIDs for inserted lines that are unique
	// Should be after the last explicit one
	TestFalse("TextID should be populated", NextNode->TextID.IsEmpty());
	TestEqual("TextID should be correct", NextNode->TextID, "@0008@");
	TestGetParsedNextNode(this, "Next", NextNode, Importer, false, &NextNode);
	if (TestParsedChoice(this, "First choice", NextNode, 2))
	{
		TestFalse("TextID should be populated", NextNode->Edges[0].TextID.IsEmpty());
		TestEqual("TextID should be correct", NextNode->Edges[0].TextID, "@0009@");
		TestFalse("TextID should be populated", NextNode->Edges[1].TextID.IsEmpty());
		TestEqual("TextID should be correct", NextNode->Edges[1].TextID, "@0001@");

		TestParsedChoiceEdge(this, "First choice", NextNode, 0, "Er, hi?", Importer, &NextNode);
		TestParsedText(this, "Next node", NextNode, "Vagabond", "Verily, 'tis wondrous to see such a fine fellow on the road this morn!");
		TestFalse("TextID should be populated", NextNode->TextID.IsEmpty());
		TestEqual("TextID should be correct", NextNode->TextID, "@000a@");
		
		TestGetParsedNextNode(this, "Next", NextNode, Importer, false, &NextNode);
		TestParsedGoto(this, "Goto", NextNode, Importer, &NextNode);
		TestParsedText(this, "Next node", NextNode, "Vagabond", "Mayhaps we could travel together a while, and share a tale or two?\nWhat do you say?");
		TestEqual("TextID should be correct", NextNode->TextID, "@0007@");
	}

	NextNode = Importer.GetNode(0);
	TestGetParsedNextNode(this, "Next", NextNode, Importer, false, &NextNode);
	if (TestParsedChoice(this, "First choice", NextNode, 2))
	{
		TestEqual("TextID should be correct", NextNode->Edges[1].TextID, "@0001@");
		TestParsedChoiceEdge(this, "First choice", NextNode, 1, "Jog on, mate", Importer, &NextNode);
		TestParsedText(this, "Next node", NextNode, "Vagabond", "Well, really! Good day then sir!");
		TestEqual("TextID should be correct", NextNode->TextID, "@0002@");
	}
	
	return true;
}

const FString ProblemChoiceInput = R"RAWSUD(

NPC: Well, hello there. This is a test.
:choice
  * A test?
		Player: Yes!
		* This is a mistake; goto goes direct to another choice
			[goto choice]
NPC: Bye!
)RAWSUD";

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestParseChoiceProblem,
								 "SUDSTest.TestParseChoiceProblem",
								 EAutomationTestFlags::EditorContext |
								 EAutomationTestFlags::ClientContext |
								 EAutomationTestFlags::ProductFilter)


bool FTestParseChoiceProblem::RunTest(const FString& Parameters)
{
	FSUDSMessageLogger Logger(false);
	FSUDSScriptImporter Importer;
	TestFalse("Import should fail", Importer.ImportFromBuffer(GetData(ProblemChoiceInput), ProblemChoiceInput.Len(), "ProblemChoiceInput", &Logger, true));

	if (TestTrue("Logger should have registered errors", Logger.HasErrors()))
	{
		FText ErrMsg = Logger.GetErrorMessages()[0]->ToText();
		TestTrue("Error should contain 'Choices MUST show another speaker line'", ErrMsg.ToString().Contains("Choices MUST show another speaker line"));
	}
	return true;
}

const FString TrailingEventInput = R"RAWSUD(

NPC: Well, hello there. This is a test.
* Choice
	[event SomeEvent 1]
)RAWSUD";

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestTrailingEventProblem,
								 "SUDSTest.TestTrailingEventProblem",
								 EAutomationTestFlags::EditorContext |
								 EAutomationTestFlags::ClientContext |
								 EAutomationTestFlags::ProductFilter)


bool FTestTrailingEventProblem::RunTest(const FString& Parameters)
{
	FSUDSMessageLogger Logger(false);
	FSUDSScriptImporter Importer;
	TestTrue("Import should succeed", Importer.ImportFromBuffer(GetData(TrailingEventInput), TrailingEventInput.Len(), "TrailingEventInput", &Logger, true));

	return true;
}

UE_ENABLE_OPTIMIZATION