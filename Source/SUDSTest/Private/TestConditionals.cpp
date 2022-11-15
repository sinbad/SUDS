#include "SUDSScriptImporter.h"
#include "TestUtils.h"
PRAGMA_DISABLE_OPTIMIZATION

const FString BasicConditionalInput = R"RAWSUD(
Player: Hello
[if {x} = 1]
    NPC: Reply when x == 1
    [if {y} == 1]
        Player: Player text when x ==1 and y == 1
    [endif]
[elseif {x} == 2]
    NPC: Reply when x == 2
[else]
    NPC: Reply when x is something else
[endif]
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
    auto RootNode = Importer.GetNode(0);
    if (!TestNotNull("Root node should exist", RootNode))
        return false;

    TestParsedText(this, "First node", RootNode, "Player", "Hello");
    
	return true;
}

PRAGMA_ENABLE_OPTIMIZATION