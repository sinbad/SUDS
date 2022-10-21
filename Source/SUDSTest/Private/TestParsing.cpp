#include "Misc/AutomationTest.h"
#include "SUDSScriptImporter.h"

const FString SimpleParsingInput = R"RAWSUD(===
# Nothing in header but a comment
===

# A comment in body

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
	// Make the test pass by returning true, or fail by returning false.
	return true;
}
