#include "SUDSDialogue.h"
#include "SUDSLibrary.h"
#include "SUDSMessageLogger.h"
#include "SUDSScript.h"
#include "SUDSScriptImporter.h"
#include "TestUtils.h"
#include "Internationalization/StringTableCore.h"
#include "Misc/AutomationTest.h"

UE_DISABLE_OPTIMIZATION

// Assign known keys to the strings so we can detect 
const FString MetadataInput = R"RAWSUD(
# No metadata
Player: Hello there @001@
#= Transient metadata for next line
#= TransientData: Something here
NPC: Hello @002@
# Should have no metadata anymore
Player: Well this is nice @003@
#+ Persistent metadata
#+ PersistentData: Something longer lived
NPC: Isn't it though @004@
# Should still apply here
Player: Indeed @005@
#= Test overriding temporarily
#= PersistentData: This should override
#= ExtraTransient: This should be new only for next line
NPC: Well well @006@
Player: Metadata should have gone back now @007@
#+ Persistent metadata changed now
Player: Persistent change @008@ 
	* Some choice @009@
		NPC: How rude @010@
		[goto end]
	#= Temporary comment
	* Another choice @011@
		NPC: This should be back now @012@
	#+ Persistent indented change 1
	* One more choice @013@
		#+ Even more indented
        #+ NestedKey: This will disappear fast even though persistent
		NPC: Ooops @014@
	* Final choice @015@
		NPC: This is going to return to the choice via goto @016@
# Nested meta should have been lost but outer meta should be back
Player: Something something @017@
)RAWSUD";

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestMetadata,
								 "SUDSTest.TestMetadata",
								 EAutomationTestFlags::EditorContext |
								 EAutomationTestFlags::ClientContext |
								 EAutomationTestFlags::ProductFilter)



bool FTestMetadata::RunTest(const FString& Parameters)
{
	FSUDSMessageLogger Logger(false);
	FSUDSScriptImporter Importer;
	TestTrue("Import should succeed", Importer.ImportFromBuffer(GetData(MetadataInput), MetadataInput.Len(), "MetadataInput", &Logger, true));

	auto Script = NewObject<USUDSScript>(GetTransientPackage(), "Test");
	const ScopedStringTableHolder StringTableHolder;
	Importer.PopulateAsset(Script, StringTableHolder.StringTable);

	// In this test we're only interested in the string table metadata
	// Have to get the mutable string table to get at metadata
	auto StrTable = StringTableHolder.StringTable->GetMutableStringTable();

	TestEqual("Line 1 Comment", StrTable->GetMetaData(FTextKey("@001@"), FName("Comment")), "");
	TestEqual("Line 1 Speaker", StrTable->GetMetaData(FTextKey("@001@"), FName("Speaker")), "Player");
	TestEqual("Line 2 Comment", StrTable->GetMetaData(FTextKey("@002@"), FName("Comment")), "Transient metadata for next line");
	TestEqual("Line 2 Speaker", StrTable->GetMetaData(FTextKey("@002@"), FName("Speaker")), "NPC");
	TestEqual("Line 2 Custom", StrTable->GetMetaData(FTextKey("@002@"), FName("TransientData")), "Something here");
	TestEqual("Line 3 Comment (should have reset)", StrTable->GetMetaData(FTextKey("@003@"), FName("Comment")), "");
	TestEqual("Line 3 Speaker", StrTable->GetMetaData(FTextKey("@003@"), FName("Speaker")), "Player");
	TestEqual("Line 3 Custom (should have reset)", StrTable->GetMetaData(FTextKey("@003@"), FName("TransientData")), "");
	TestEqual("Line 4 Comment", StrTable->GetMetaData(FTextKey("@004@"), FName("Comment")), "Persistent metadata");
	TestEqual("Line 4 Speaker", StrTable->GetMetaData(FTextKey("@004@"), FName("Speaker")), "NPC");
	TestEqual("Line 4 Custom", StrTable->GetMetaData(FTextKey("@004@"), FName("PersistentData")), "Something longer lived");
	TestEqual("Line 5 Comment", StrTable->GetMetaData(FTextKey("@005@"), FName("Comment")), "Persistent metadata");
	TestEqual("Line 5 Speaker", StrTable->GetMetaData(FTextKey("@005@"), FName("Speaker")), "Player");
	TestEqual("Line 5 Custom", StrTable->GetMetaData(FTextKey("@005@"), FName("PersistentData")), "Something longer lived");
	TestEqual("Line 6 Comment", StrTable->GetMetaData(FTextKey("@006@"), FName("Comment")), "Test overriding temporarily");
	TestEqual("Line 6 Speaker", StrTable->GetMetaData(FTextKey("@006@"), FName("Speaker")), "NPC");
	TestEqual("Line 6 Custom", StrTable->GetMetaData(FTextKey("@006@"), FName("PersistentData")), "This should override");
	TestEqual("Line 6 Custom 2", StrTable->GetMetaData(FTextKey("@006@"), FName("ExtraTransient")), "This should be new only for next line");
	TestEqual("Line 7 Comment", StrTable->GetMetaData(FTextKey("@007@"), FName("Comment")), "Persistent metadata");
	TestEqual("Line 7 Speaker", StrTable->GetMetaData(FTextKey("@007@"), FName("Speaker")), "Player");
	TestEqual("Line 7 Custom", StrTable->GetMetaData(FTextKey("@007@"), FName("PersistentData")), "Something longer lived");
	TestEqual("Line 8 Comment", StrTable->GetMetaData(FTextKey("@008@"), FName("Comment")), "Persistent metadata changed now");
	TestEqual("Line 8 Speaker", StrTable->GetMetaData(FTextKey("@008@"), FName("Speaker")), "Player");
	TestEqual("Line 8 Custom", StrTable->GetMetaData(FTextKey("@008@"), FName("PersistentData")), "Something longer lived");
	TestEqual("Line 9 Comment", StrTable->GetMetaData(FTextKey("@009@"), FName("Comment")), "Persistent metadata changed now");
	TestEqual("Line 9 Speaker", StrTable->GetMetaData(FTextKey("@009@"), FName("Speaker")), "Player (Choice)");
	TestEqual("Line 9 Custom", StrTable->GetMetaData(FTextKey("@009@"), FName("PersistentData")), "Something longer lived");
	TestEqual("Line 10 Comment", StrTable->GetMetaData(FTextKey("@010@"), FName("Comment")), "Persistent metadata changed now");
	TestEqual("Line 10 Speaker", StrTable->GetMetaData(FTextKey("@010@"), FName("Speaker")), "NPC");
	TestEqual("Line 10 Custom", StrTable->GetMetaData(FTextKey("@010@"), FName("PersistentData")), "Something longer lived");
	TestEqual("Line 11 Comment", StrTable->GetMetaData(FTextKey("@011@"), FName("Comment")), "Temporary comment");
	TestEqual("Line 11 Speaker", StrTable->GetMetaData(FTextKey("@011@"), FName("Speaker")), "Player (Choice)");
	TestEqual("Line 11 Custom", StrTable->GetMetaData(FTextKey("@011@"), FName("PersistentData")), "Something longer lived");
	TestEqual("Line 12 Comment", StrTable->GetMetaData(FTextKey("@012@"), FName("Comment")), "Persistent metadata changed now");
	TestEqual("Line 12 Speaker", StrTable->GetMetaData(FTextKey("@012@"), FName("Speaker")), "NPC");
	TestEqual("Line 12 Custom", StrTable->GetMetaData(FTextKey("@012@"), FName("PersistentData")), "Something longer lived");
	TestEqual("Line 13 Comment", StrTable->GetMetaData(FTextKey("@013@"), FName("Comment")), "Persistent indented change 1");
	TestEqual("Line 13 Speaker", StrTable->GetMetaData(FTextKey("@013@"), FName("Speaker")), "Player (Choice)");
	TestEqual("Line 13 Custom", StrTable->GetMetaData(FTextKey("@013@"), FName("PersistentData")), "Something longer lived");
	TestEqual("Line 14 Comment", StrTable->GetMetaData(FTextKey("@014@"), FName("Comment")), "Even more indented");
	TestEqual("Line 14 Speaker", StrTable->GetMetaData(FTextKey("@014@"), FName("Speaker")), "NPC");
	TestEqual("Line 14 Custom", StrTable->GetMetaData(FTextKey("@014@"), FName("PersistentData")), "Something longer lived");
	TestEqual("Line 14 Custom 2", StrTable->GetMetaData(FTextKey("@014@"), FName("NestedKey")), "This will disappear fast even though persistent");
	TestEqual("Line 15 Comment", StrTable->GetMetaData(FTextKey("@015@"), FName("Comment")), "Persistent indented change 1"); // should have gone back
	TestEqual("Line 15 Speaker", StrTable->GetMetaData(FTextKey("@015@"), FName("Speaker")), "Player (Choice)");
	TestEqual("Line 15 Custom", StrTable->GetMetaData(FTextKey("@015@"), FName("PersistentData")), "Something longer lived");
	TestEqual("Line 15 Custom 2", StrTable->GetMetaData(FTextKey("@015@"), FName("NestedKey")), "");
	TestEqual("Line 16 Comment", StrTable->GetMetaData(FTextKey("@016@"), FName("Comment")), "Persistent indented change 1"); // should have gone back
	TestEqual("Line 16 Speaker", StrTable->GetMetaData(FTextKey("@016@"), FName("Speaker")), "NPC");
	TestEqual("Line 16 Custom", StrTable->GetMetaData(FTextKey("@016@"), FName("PersistentData")), "Something longer lived");
	TestEqual("Line 16 Custom 2", StrTable->GetMetaData(FTextKey("@016@"), FName("NestedKey")), "");
	TestEqual("Line 17 Comment", StrTable->GetMetaData(FTextKey("@017@"), FName("Comment")), "Persistent metadata changed now"); // should have gone back to top level
	TestEqual("Line 17 Speaker", StrTable->GetMetaData(FTextKey("@017@"), FName("Speaker")), "Player");
	TestEqual("Line 17 Custom", StrTable->GetMetaData(FTextKey("@017@"), FName("PersistentData")), "Something longer lived");
	

	Script->MarkAsGarbage();
	return true;
	
}

UE_ENABLE_OPTIMIZATION
