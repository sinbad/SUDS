#include "Misc/AutomationTest.h"
#include "SUDSScriptImporter.h"

const FString SimpleParsingInput = R"RAWSUD(===
# Nothing in header but a comment
===

# A comment in body

Player: Excuse me?
NPC: Well, hello there. This is a test.
  * A test?
    NPC: Yes, a test. This is some indented continuation text.
    Player: Oh I see, thank you.
  * Another option
    NPC: This is another option with an embedded continuation.
    * How far can this go?
      NPC: Theoretically forever but who knows?
Player: Well, that's all for now. This should appear for all paths as a fall-through.
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


	return true;
}
