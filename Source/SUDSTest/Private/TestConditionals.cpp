#include "SUDSScriptImporter.h"
#include "TestUtils.h"
#include "Tests/FbxAutomationCommon.h"
PRAGMA_DISABLE_OPTIMIZATION

const FString BasicConditionalInput = R"RAWSUD(
Player: Hello
[if {x} == 1]
    NPC: Reply when x == 1
    [if {y} == 1]
        Player: Player text when x ==1 and y == 1
    [endif]
[elseif {x} == 2]
    NPC: Reply when x == 2
[else]
    NPC: Reply when x is something else
[endif]
[if {z} == true]
    Player: the end is true
[else]
    Player: the end is false
[endif]
NPC: OK
)RAWSUD";

const FString ConditionalChoiceInput = R"RAWSUD(
# First test has a regular choice first before conditional
NPC: Hello
    * First choice
        Player: I took the 1.1 choice
[if {y} == 2]
    * Second choice (conditional)
        Player: I took the 1.2 choice
    * Third choice (conditional)
        Player: I took the 1.3 choice
[else]
    * Second Alt Choice
        Player: I took the alt 1.2 choice        
[endif]
    * Common last choice
        Player: I took the 1.4 choice
# Second test has conditional choice as the first one
NPC: OK next question
[if {y} == 0]
    * First conditional choice
        Player: I took the 2.1 choice
[elseif {y} == 1]
    * Second conditional choice
        Player: I took the 2.2 choice
    [if {q} == 10]
        * Nested conditional choice
            Player: I took the 2.2.1 choice
    [endif]
[else]
    * Third conditional choice
        Player: I took the 2.3 choice
[endif]
    * Final common choice
        Player: I took the 2.4 choice
NPC: Bye
)RAWSUD";



IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestBasicConditionals,
								 "SUDSTest.TestBasicConditionals",
								 EAutomationTestFlags::EditorContext |
								 EAutomationTestFlags::ClientContext |
								 EAutomationTestFlags::ProductFilter)


bool FTestBasicConditionals::RunTest(const FString& Parameters)
{
    FSUDSScriptImporter Importer;
    TestTrue("Import should succeed", Importer.ImportFromBuffer(GetData(BasicConditionalInput), BasicConditionalInput.Len(), "BasicConditionalInput", true));

    // Test the content of the parsing
    auto NextNode = Importer.GetNode(0);
    if (!TestNotNull("Root node should exist", NextNode))
        return false;

    TestParsedText(this, "First node", NextNode, "Player", "Hello");
    TestGetParsedNextNode(this, "Get next", NextNode, Importer, false, &NextNode);

    if (TestParsedSelect(this, "First Select node", NextNode, 3))
    {
        auto SelectNode = NextNode;
        TestParsedSelectEdge(this, "First select edge 1 (if)", SelectNode, 0, "{x} == 1", Importer, &NextNode);
        TestParsedText(this, "Nested node 1", NextNode, "NPC", "Reply when x == 1");
        TestGetParsedNextNode(this, "Get next", NextNode, Importer, false, &NextNode);
        // Note: even though this is a single "if", there is an implicit "else" edge created
        if (TestParsedSelect(this, "Nested Select node", NextNode, 2))
        {
            // Nested select
            auto SelectNode2 = NextNode;
            TestParsedSelectEdge(this, "Nested select edge 1", SelectNode2, 0, "{y} == 1", Importer, &NextNode);
            TestParsedText(this, "Nested node edge 1", NextNode, "Player", "Player text when x ==1 and y == 1");
            TestGetParsedNextNode(this, "Get next", NextNode, Importer, false, &NextNode);
            if (TestParsedSelect(this, "Fallthrough select", NextNode, 2))
            {
                auto SelectNode3 = NextNode;
                TestParsedSelectEdge(this, "Final select edge 1", SelectNode3, 0, "{z} == true", Importer, &NextNode);
                TestParsedText(this, "Final select edge 1 text", NextNode, "Player", "the end is true");
                TestGetParsedNextNode(this, "Get next", NextNode, Importer, false, &NextNode);
                TestParsedText(this, "Final fallthrough", NextNode, "NPC", "OK");

                TestParsedSelectEdge(this, "Final select edge 2", SelectNode3, 1, "", Importer, &NextNode);
                TestParsedText(this, "Final select edge 2 text", NextNode, "Player", "the end is false");
                TestGetParsedNextNode(this, "Get next", NextNode, Importer, false, &NextNode);
                TestParsedText(this, "Final fallthrough", NextNode, "NPC", "OK");
                
            }

            // Go back to the nested select
            // This "else" edge should have been created automatically to fall through 
            TestParsedSelectEdge(this, "Nested select edge 2", SelectNode2, 1, "", Importer, &NextNode);
            // Just test it gets to the fallthrough, we've already tested the continuation from there
            TestParsedSelect(this, "Fallthrough select", NextNode, 2);
            
        }
        TestParsedSelectEdge(this, "First select edge 2 (elseif)", SelectNode, 1, "{x} == 2", Importer, &NextNode);
        TestParsedText(this, "Select node 2", NextNode, "NPC", "Reply when x == 2");
        TestGetParsedNextNode(this, "Get next", NextNode, Importer, false, &NextNode);
        // Just test it gets to the fallthrough, we've already tested the continuation from there
        TestParsedSelect(this, "Fallthrough select", NextNode, 2);
        
        TestParsedSelectEdge(this, "First select edge 2 (else)", SelectNode, 2, "", Importer, &NextNode);
        TestParsedText(this, "Select node 3", NextNode, "NPC", "Reply when x is something else");
        TestGetParsedNextNode(this, "Get next", NextNode, Importer, false, &NextNode);
        // Just test it gets to the fallthrough, we've already tested the continuation from there
        TestParsedSelect(this, "Fallthrough select", NextNode, 2);
    }
    
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestConditionalChoices,
                                 "SUDSTest.TestConditionalChoices",
                                 EAutomationTestFlags::EditorContext |
                                 EAutomationTestFlags::ClientContext |
                                 EAutomationTestFlags::ProductFilter)


bool FTestConditionalChoices::RunTest(const FString& Parameters)
{
    FSUDSScriptImporter Importer;
    TestTrue("Import should succeed", Importer.ImportFromBuffer(GetData(ConditionalChoiceInput), ConditionalChoiceInput.Len(), "ConditionalChoiceInput", true));

    // Test the content of the parsing
    auto NextNode = Importer.GetNode(0);
    if (!TestNotNull("Root node should exist", NextNode))
        return false;

    TestParsedText(this, "First node", NextNode, "NPC", "Hello");
    TestGetParsedNextNode(this, "Get next", NextNode, Importer, false, &NextNode);
    // This will have one choice edge for the unconditional choice, then an edge to the select node
    if (TestParsedChoice(this, "First choice node", NextNode, 3))
    {
        auto ChoiceNode = NextNode;
        TestParsedChoiceEdge(this, "First choice edge 1", ChoiceNode, 0, "First choice", Importer, &NextNode);
        TestParsedText(this, "First choice edge 1 text", NextNode, "Player", "I took the 1.1 choice");
        TestGetParsedNextNode(this, "Get next", NextNode, Importer, false, &NextNode);
        TestParsedText(this, "Fallthrough to next question", NextNode, "NPC", "OK next question");
        
        // Second edge has no text because it's going to a select for other choices
        TestParsedChoiceEdge(this, "First choice edge 2", ChoiceNode, 1, "", Importer, &NextNode);
        // 2 edges because only if/else
        if (TestParsedSelect(this, "First choice edge 2 should link to select", NextNode, 2))
        {
            auto SelectNode = NextNode;
            TestParsedSelectEdge(this, "Select edge 0", SelectNode, 0, "{y} == 2", Importer, &NextNode);
            // 2 choices in this if clause
            if (TestParsedChoice(this, "Choice node under first if", NextNode, 2))
            {
                auto Choice2 = NextNode;
                TestParsedChoiceEdge(this, "y == 2 choice edge 1", Choice2, 0, "Second choice (conditional)", Importer, &NextNode);
                TestParsedText(this, "y == 2 choice edge 1 text", NextNode, "Player", "I took the 1.2 choice");
                // This should fall through to text node after the choices
                TestGetParsedNextNode(this, "Get next", NextNode, Importer, false, &NextNode);
                TestParsedText(this, "Fallthrough to next text", NextNode, "NPC", "OK next question");

                TestParsedChoiceEdge(this, "y == 2 choice edge 2", Choice2, 1, "Third choice (conditional)", Importer, &NextNode);
                TestParsedText(this, "y == 2 choice edge 2 text", NextNode, "Player", "I took the 1.3 choice");
                // This should fall through to text node after the choices
                TestGetParsedNextNode(this, "Get next", NextNode, Importer, false, &NextNode);
                TestParsedText(this, "Fallthrough to next text", NextNode, "NPC", "OK next question");

            }
            TestParsedSelectEdge(this, "Select edge 1 (else)", SelectNode, 1, "", Importer, &NextNode);
            // 1 choice under this else section
            if (TestParsedChoice(this, "Choice node under first else", NextNode, 1))
            {
                auto Choice3 = NextNode;
                TestParsedChoiceEdge(this, "y == 2 choice edge 1", Choice3, 0, "Second Alt Choice", Importer, &NextNode);
                TestParsedText(this, "Else edge text", NextNode, "Player", "I took the alt 1.2 choice");
                TestGetParsedNextNode(this, "Get next", NextNode, Importer, false, &NextNode);
                TestParsedText(this, "Fallthrough to next question", NextNode, "NPC", "OK next question");
            }

            // Final fallthrough choice, unconditional
            TestParsedChoiceEdge(this, "First choice edge 3", ChoiceNode, 2, "Common last choice", Importer, &NextNode);
            TestParsedText(this, "Fallthrough to next text", NextNode, "Player", "I took the 1.4 choice");
            TestGetParsedNextNode(this, "Get next", NextNode, Importer, false, &NextNode);
            TestParsedText(this, "Fallthrough to next question", NextNode, "NPC", "OK next question");
            
            TestGetParsedNextNode(this, "Get next", NextNode, Importer, false, &NextNode);
            // This time because the first choice is already in the select, the select comes first
            // There's an if, elseif, and else edge 
            if (TestParsedSelect(this, "Select node under next question", NextNode, 3))
            {
                auto SelectNode2 = NextNode;
                TestParsedSelectEdge(this, "Select edge 0", SelectNode2, 0, "{y} == 0", Importer, &NextNode);
                // Choice node under if, only 1 in this section
                if (TestParsedChoice(this, "Choice node under select", NextNode, 1))
                {
                    TestParsedChoiceEdge(this, "First choice edge", NextNode, 0, "First conditional choice", Importer, &NextNode);
                    TestParsedText(this, "First choice text", NextNode, "Player", "I took the 2.1 choice");
                    TestGetParsedNextNode(this, "Get next", NextNode, Importer, false, &NextNode);
                    TestParsedText(this, "Final fallthrough", NextNode, "NPC", "Bye");
                    
                }
                TestParsedSelectEdge(this, "Select edge 1", SelectNode2, 1, "{y} == 1", Importer, &NextNode);
                // Choice node under elseif, only 1 in this section
                if (TestParsedChoice(this, "Choice node under select 2", NextNode, 2))
                {
                    auto Choice4 = NextNode;
                    TestParsedChoiceEdge(this, "Second choice edge 1", Choice4, 0, "Second conditional choice", Importer, &NextNode);
                    TestParsedText(this, "First choice text", NextNode, "Player", "I took the 2.2 choice");
                    TestGetParsedNextNode(this, "Get next", NextNode, Importer, false, &NextNode);
                    TestParsedText(this, "Final fallthrough", NextNode, "NPC", "Bye");

                    // Next edge goes to a select
                    TestParsedChoiceEdge(this, "Second choice edge 2", Choice4, 1, "", Importer, &NextNode);
                    // There is only an "if" in this nested select. No implicit "else" edge should exist because it's a choice node 
                    if (TestParsedSelect(this, "Select node under next question", NextNode, 1))
                    {
                    }
                    
                }
                
                
            }
            

            
        }
        
    }
    
    
    return true;
}

PRAGMA_ENABLE_OPTIMIZATION