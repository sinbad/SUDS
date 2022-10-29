#include "SUDSScript.h"
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
		NPC: Some nesting
		* Go to goodbye
			Player: Gotta go!
			[go to goodbye] 
		* Skip
			[goto secondchoice]
		* This is a mistake
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
	
	TestEqual("Root node type", RootNode->NodeType, ESUDSParsedNodeType::Text);
	TestEqual("Root node speaker", RootNode->SpeakerOrGotoLabel, "Player");
	TestEqual("Root node text", RootNode->Text, "Excuse me?");
	TestEqual("Root node edges", RootNode->Edges.Num(), 1);

	auto NextNode = Importer.GetNode(RootNode->Edges[0].TargetNodeIdx);
	if (!TestNotNull("Next node should exist", NextNode))
		return false;

	TestEqual("Second node type", NextNode->NodeType, ESUDSParsedNodeType::Text);
	TestEqual("Second node speaker", NextNode->SpeakerOrGotoLabel, "NPC");
	TestEqual("Second node text", NextNode->Text, "Well, hello there. This is a test.");
	TestEqual("Second node edges", NextNode->Edges.Num(), 1);

	NextNode = Importer.GetNode(NextNode->Edges[0].TargetNodeIdx);
	if (!TestNotNull("Third node should exist", NextNode))
		return false;
	TestEqual("Third node type", NextNode->NodeType, ESUDSParsedNodeType::Choice);
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
			TestEqual("Choice 1 1st text node speaker", NextNode->SpeakerOrGotoLabel, "NPC");
			TestEqual("Choice 1 1st text node text", NextNode->Text, "Yes, a test. This is some indented continuation text.");
			TestEqual("Choice 1 1st text node edges", NextNode->Edges.Num(), 1);
			NextNode = Importer.GetNode(NextNode->Edges[0].TargetNodeIdx);
			if (TestNotNull("Next node should exist", NextNode))
			{
				TestEqual("Choice 1 2nd text node type", NextNode->NodeType, ESUDSParsedNodeType::Text);
				TestEqual("Choice 1 2nd text node speaker", NextNode->SpeakerOrGotoLabel, "Player");
				TestEqual("Choice 1 2nd text node text", NextNode->Text, "Oh I see, thank you.");
				TestEqual("Choice 1 2nd text node edges", NextNode->Edges.Num(), 1);
				NextNode = Importer.GetNode(NextNode->Edges[0].TargetNodeIdx);
				if (TestNotNull("Next node should exist", NextNode))
				{
					TestEqual("Choice 1 3rd text node type", NextNode->NodeType, ESUDSParsedNodeType::Text);
					TestEqual("Choice 1 3rd text node speaker", NextNode->SpeakerOrGotoLabel, "NPC");
					TestEqual("Choice 1 3rd text node text", NextNode->Text, "You're welcome.");

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
		TestEqual("Choice 2 1st text node speaker", NextNode->SpeakerOrGotoLabel, "NPC");
		TestEqual("Choice 2 1st text node text", NextNode->Text, "This is another option with an embedded choice.");
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
				TestEqual("Nested Choice 1st text node speaker", NextNode->SpeakerOrGotoLabel, "NPC");
				TestEqual("Nested Choice 1st text node text", NextNode->Text, "Theoretically forever but who knows?");
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
				TestEqual("Nested Choice 2nd text node speaker", NextNode->SpeakerOrGotoLabel, "NPC");
				TestEqual("Nested Choice 2nd text node text", NextNode->Text, "That should have been added to the previous choice");
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
				TestEqual("Nested Choice 3rd text node speaker", NextNode->SpeakerOrGotoLabel, "NPC");
				TestEqual("Nested Choice 3rd text node text", NextNode->Text, "Yep, this one too");
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
								TestEqual("Double Nested Choice option 0 text node speaker", NextNode->SpeakerOrGotoLabel, "NPC");
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
								TestEqual("Double Nested Choice option 1 text node speaker", NextNode->SpeakerOrGotoLabel, "NPC");
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
			TestEqual("Fallthrough node speaker", FallthroughNode->SpeakerOrGotoLabel, "Player");
			TestEqual("Fallthrough node text", FallthroughNode->Text, "Well, that's all for now. This should appear for all paths as a fall-through.\nThis, in fact, is a multi-line piece of text\nWhich is joined to the previous text node with the line breaks");
			if (TestEqual("Fallthrough node edge count", FallthroughNode->Edges.Num(), 1))
			{
				NextNode = Importer.GetNode(FallthroughNode->Edges[0].TargetNodeIdx);
				if (TestNotNull("Fallthrough node next node not null", NextNode))
				{
					TestEqual("Fallthrough node 2 type", NextNode->NodeType, ESUDSParsedNodeType::Text);
					TestEqual("Fallthrough node 2 speaker", NextNode->SpeakerOrGotoLabel, "NPC");
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
	FSUDSScriptImporter Importer;
	TestTrue("Import should succeed", Importer.ImportFromBuffer(GetData(GotoParsingInput), GotoParsingInput.Len(), "GotoParsingInput", true));

	// Test the content of the parsing
	auto RootNode = Importer.GetNode(0);
	if (!TestNotNull("Root node should exist", RootNode))
		return false;

	TestEqual("Root node type", RootNode->NodeType, ESUDSParsedNodeType::Text);
	TestEqual("Root node speaker", RootNode->SpeakerOrGotoLabel, "Player");
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
			TestEqual("Goto End node text speaker", NextNode->SpeakerOrGotoLabel, "NPC");
			TestEqual("Goto End node text text", NextNode->Text, "How rude, bye then");
			if (TestEqual("Goto End node text edges", NextNode->Edges.Num(), 1))
			{
				NextNode = Importer.GetNode(NextNode->Edges[0].TargetNodeIdx);
				if (TestNotNull("Goto node should not be null", NextNode))
				{
					TestEqual("Goto End node type", NextNode->NodeType, ESUDSParsedNodeType::Goto);
					TestEqual("Goto End node label", NextNode->SpeakerOrGotoLabel, FSUDSScriptImporter::EndGotoLabel);
				}
				
			}
			
		}
		TestEqual("Choice 2 text", ChoiceNode->Edges[1].Text, "Nested option");
		NextNode = Importer.GetNode(ChoiceNode->Edges[1].TargetNodeIdx);
		if (TestNotNull("Next node should not be null", NextNode))
		{
			TestEqual("Choice 2 node text type", NextNode->NodeType, ESUDSParsedNodeType::Text);
			TestEqual("Choice 2 node text speaker", NextNode->SpeakerOrGotoLabel, "NPC");
			TestEqual("Choice 2 node text text", NextNode->Text, "Some nesting");
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
							TestEqual("Nested choice 0 text speaker", NextNode->SpeakerOrGotoLabel, "Player");
							TestEqual("Nested choice 0 text text", NextNode->Text, "Gotta go!");

							if (TestEqual("Nested Choice textnode edges", NextNode->Edges.Num(), 1))
							{
								NextNode = Importer.GetNode(NextNode->Edges[0].TargetNodeIdx);
								if (TestNotNull("Should not be null", NextNode))
								{
									// This is the goto
									TestEqual("Nested choice 0 goto type", NextNode->NodeType, ESUDSParsedNodeType::Goto);
									TestEqual("Nested choice 0 goto label", NextNode->SpeakerOrGotoLabel, "goodbye");
									const int DestIdx = Importer.GetGotoTargetNodeIndex(NextNode->SpeakerOrGotoLabel);
									TestNotEqual("Label should be valid", DestIdx, -1);
									NextNode = Importer.GetNode(DestIdx);
									if (TestNotNull("Goto dest node", NextNode))
									{
										TestEqual("Goto dest text type", NextNode->NodeType, ESUDSParsedNodeType::Text);
										TestEqual("Goto dest text speaker", NextNode->SpeakerOrGotoLabel, "NPC");
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
							TestEqual("Nested choice 1 goto label", NextNode->SpeakerOrGotoLabel, "secondchoice");

							const int DestIdx = Importer.GetGotoTargetNodeIndex(NextNode->SpeakerOrGotoLabel);
							TestNotEqual("Label should be valid", DestIdx, -1);
							NextNode = Importer.GetNode(DestIdx);
							if (TestNotNull("Goto dest node", NextNode))
							{
								TestEqual("Goto dest text type", NextNode->NodeType, ESUDSParsedNodeType::Text);
								TestEqual("Goto dest text speaker", NextNode->SpeakerOrGotoLabel, "NPC");
								TestEqual("Goto dest text text", NextNode->Text, "Yep, this one too");
										
							}
						}
						TestEqual("Nested choice 2 text", NestedChoice->Edges[2].Text, "This is a mistake");
						NextNode = Importer.GetNode(NestedChoice->Edges[2].TargetNodeIdx);
						if (TestNotNull("Should not be null", NextNode))
						{
							TestEqual("Nested choice 2 text type", NextNode->NodeType, ESUDSParsedNodeType::Text);
							TestEqual("Nested choice 2 text speaker", NextNode->SpeakerOrGotoLabel, "NPC");
							TestEqual("Nested choice 2 text text", NextNode->Text, "Oh no");

							if (TestEqual("Nested Choice text node edges", NextNode->Edges.Num(), 1))
							{
								NextNode = Importer.GetNode(NextNode->Edges[0].TargetNodeIdx);
								if (TestNotNull("Should not be null", NextNode))
								{
									// This is the goto
									TestEqual("Nested choice 0 goto type", NextNode->NodeType, ESUDSParsedNodeType::Goto);
									TestEqual("Nested choice 0 goto label", NextNode->SpeakerOrGotoLabel, "this_is_an_error");
									TestEqual("Label should go nowhere", Importer.GetGotoTargetNodeIndex(NextNode->SpeakerOrGotoLabel), -1);
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
			TestEqual("End choice node text speaker", NextNode->SpeakerOrGotoLabel, "NPC");
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
							TestEqual("Node text speaker", NextNode->SpeakerOrGotoLabel, "NPC");
							TestEqual("Node text text", NextNode->Text, "Okay!");
							if (TestEqual("Next node edges", NextNode->Edges.Num(), 1))
							{
								NextNode = Importer.GetNode(NextNode->Edges[0].TargetNodeIdx);
								if (TestNotNull("", NextNode))
								{
									TestEqual("Should be goto node", NextNode->NodeType, ESUDSParsedNodeType::Goto);
									TestEqual("Goto label", NextNode->SpeakerOrGotoLabel, "choice");
									
									// This should lead back to a choice node, ie letting the previous text node use the same choices
									// without repeating the text (loop with context)
									const int DestIdx = Importer.GetGotoTargetNodeIndex(NextNode->SpeakerOrGotoLabel);
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
								TestEqual("Node text speaker", NextNode->SpeakerOrGotoLabel, "NPC");
								TestEqual("Node text text", NextNode->Text, "Gotcha");
								if (TestEqual("Next node edges", NextNode->Edges.Num(), 1))
								{
									NextNode = Importer.GetNode(NextNode->Edges[0].TargetNodeIdx);
									if (TestNotNull("", NextNode))
									{
										TestEqual("Should be goto node", NextNode->NodeType, ESUDSParsedNodeType::Goto);
										TestEqual("Goto label", NextNode->SpeakerOrGotoLabel, "start");
									
										// This should lead back to a choice node, ie letting the previous text node use the same choices
										// without repeating the text (loop with context)
										const int DestIdx = Importer.GetGotoTargetNodeIndex(NextNode->SpeakerOrGotoLabel);
										TestNotEqual("Label should be valid", DestIdx, -1);
										NextNode = Importer.GetNode(DestIdx);
										if (TestNotNull("Goto dest node", NextNode))
										{
											// Just make sure it's the right node
											TestEqual("Goto dest text type", NextNode->NodeType, ESUDSParsedNodeType::Text);
											TestEqual("Node text speaker", NextNode->SpeakerOrGotoLabel, "Player");
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
								TestEqual("Goto label", NextNode->SpeakerOrGotoLabel, "alsostart");
							
								// This should lead back to a choice node, ie letting the previous text node use the same choices
								// without repeating the text (loop with context)
								const int DestIdx = Importer.GetGotoTargetNodeIndex(NextNode->SpeakerOrGotoLabel);
								TestNotEqual("Label should be valid", DestIdx, -1);
								NextNode = Importer.GetNode(DestIdx);
								if (TestNotNull("Goto dest node", NextNode))
								{
									// Just make sure it's the right node
									TestEqual("Goto dest text type", NextNode->NodeType, ESUDSParsedNodeType::Text);
									TestEqual("Node text speaker", NextNode->SpeakerOrGotoLabel, "Player");
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
			TestEqual("Start node text speaker", StartNode->SpeakerOrGotoLabel, "Player");
			TestEqual("Start node text text", StartNode->Text, "This is the start");
		}
		const int AlsoStartNodeIdx = Importer.GetGotoTargetNodeIndex("alsostart");
		TestEqual("start and alsostart should reference the same node", AlsoStartNodeIdx, StartNodeIdx);
		
	}
	
	return true;
}


bool TestTextNode(FAutomationTestBase* T, const FString& NameForTest, const USUDSScriptNode* Node, const FString& Speaker, const FString& Text)
{
	if (T->TestNotNull(NameForTest, Node))
	{
		T->TestEqual(NameForTest, Node->GetNodeType(), ESUDSScriptNodeType::Text);
		T->TestEqual(NameForTest, Node->GetSpeakerID(), Speaker);
		T->TestEqual(NameForTest, Node->GetTextID(), Text);
		return true;
	}
	return false;
}
bool TestEdge(FAutomationTestBase* T, const FString& NameForTest, USUDSScriptNode* Node, int EdgeIndex, USUDSScriptNode** OutNode)
{
	*OutNode = nullptr;
	if (Node && Node->GetEdgeCount() > EdgeIndex)
	{
		if (auto Edge = Node->GetEdge(EdgeIndex))
		{
			*OutNode = Edge->TargetNode.Get();
			return T->TestNotNull(NameForTest, *OutNode);
		}
	}
	
	return false;
	
}

bool TestChoiceNode(FAutomationTestBase* T, const FString& NameForTest, const USUDSScriptNode* Node, int NumChoices)
{
	if (T->TestNotNull(NameForTest, Node))
	{
		T->TestEqual(NameForTest, Node->GetNodeType(), ESUDSScriptNodeType::Choice);
		return T->TestEqual(NameForTest, Node->GetEdgeCount(), NumChoices);
	}
	return false;
}

bool TestChoiceEdge(FAutomationTestBase* T, const FString& NameForTest, USUDSScriptNode* Node, int EdgeIndex, const FString& Text, USUDSScriptNode** OutNode)
{
	*OutNode = nullptr;
	if (Node && Node->GetEdgeCount() > EdgeIndex)
	{
		if (auto Edge = Node->GetEdge(EdgeIndex))
		{
			T->TestEqual(NameForTest, Edge->Navigation, ESUDSScriptEdgeNavigation::Explicit);
			T->TestEqual(NameForTest, Edge->TempText, Text);
			*OutNode = Edge->TargetNode.Get();
			return T->TestNotNull(NameForTest, *OutNode);
		}
	}
	
	return false;
	
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestConversionToRuntime,
								 "SUDSTest.TestConversionToRuntime",
								 EAutomationTestFlags::EditorContext |
								 EAutomationTestFlags::ClientContext |
								 EAutomationTestFlags::ProductFilter)


bool FTestConversionToRuntime::RunTest(const FString& Parameters)
{
	FSUDSScriptImporter Importer;
	TestTrue("Import should succeed", Importer.ImportFromBuffer(GetData(GotoParsingInput), GotoParsingInput.Len(), "GotoParsingInput", true));

	auto Asset = NewObject<USUDSScript>(GetTransientPackage(), "Test");
	Importer.PopulateAsset(Asset);

	auto StartNode = Asset->GetFirstNode();
	if (!TestNotNull("Start node should be true", StartNode))
		return false;

	TestTextNode(this, "Start node", StartNode, "Player", "This is the start");

	auto NextNode = StartNode;
	if (!TestEqual("Start node edges", NextNode->GetEdgeCount(), 1))
		return false;
	
	auto pEdge = NextNode->GetEdge(0);
	if (!TestNotNull("Start node edge", pEdge))
		return false;
	
	TestEqual("Start node edge", pEdge->Navigation, ESUDSScriptEdgeNavigation::Combine);
	NextNode = pEdge->TargetNode.Get();
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
				TestFalse("Goto end should be null", GotoEdge->TargetNode.IsValid());
			}
		}
		
		if (TestChoiceEdge(this, "Choice 2 text", ChoiceNode, 1, "Nested option", &NextNode))
		{
			TestTextNode(this, "Choice 2 node text type", NextNode, "NPC", "Some nesting");
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

					if (TestChoiceEdge(this, "Nested choice 2 text", NestedChoice, 2, "This is a mistake", &NextNode))
					{
						// there's a text node then a failed goto (nowhere)
						TestTextNode(this, "Nested choice 2 node", NextNode, "NPC", "Oh no");
						auto GotoEdge = NextNode->GetEdge(0);
						if (TestNotNull("Nested choice 2 node edge", GotoEdge))
						{
							TestFalse("Nested choice 2 node goes nowhere", GotoEdge->TargetNode.IsValid());
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
	
	
	return true;
}
PRAGMA_ENABLE_OPTIMIZATION