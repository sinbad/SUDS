#include "SUDSScriptImporter.h"
#include "TestUtils.h"
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
Player: the end
)RAWSUD";

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestBasicConditionals,
								 "SUDSTest.TestConditionals",
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
            TestParsedText(this, "Nested node edge fallthrough", NextNode, "Player", "the end");

            // Go back to the nested select
            // This "else" edge should have been created automatically to fall through 
            TestParsedSelectEdge(this, "Nested select edge 2", SelectNode2, 1, "", Importer, &NextNode);
            TestParsedText(this, "Nested node edge fallthrough", NextNode, "Player", "the end");
            
        }
        TestParsedSelectEdge(this, "First select edge 2 (elseif)", SelectNode, 1, "{x} == 2", Importer, &NextNode);
        TestParsedText(this, "Select node 2", NextNode, "NPC", "Reply when x == 2");
        TestGetParsedNextNode(this, "Get next", NextNode, Importer, false, &NextNode);
        TestParsedText(this, "Elseif node edge fallthrough", NextNode, "Player", "the end");
        
        TestParsedSelectEdge(this, "First select edge 2 (else)", SelectNode, 2, "", Importer, &NextNode);
        TestParsedText(this, "Select node 3", NextNode, "NPC", "Reply when x is something else");
        TestGetParsedNextNode(this, "Get next", NextNode, Importer, false, &NextNode);
        TestParsedText(this, "Else node edge fallthrough", NextNode, "Player", "the end");
    }
    
	return true;
}

PRAGMA_ENABLE_OPTIMIZATION