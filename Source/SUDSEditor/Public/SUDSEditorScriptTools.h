// Copyright Steve Streeting 2022
// Released under the MIT license https://opensource.org/license/MIT/
#pragma once

#include "CoreMinimal.h"

class USUDSScript;
class USUDSScriptNode;
struct FSUDSMessageLogger;

class FSUDSEditorScriptTools
{
public:

	static void WriteBackTextIDs(USUDSScript* Script, FSUDSMessageLogger& Logger);
	static bool WriteBackTextIDsFromNodes(const TArray<USUDSScriptNode*> Nodes, TArray<FString>& Lines, const FString& NameForErrors, FSUDSMessageLogger& Logger);
	static bool WriteBackTextID(const FText& AssetText, int LineNo, TArray<FString>& Lines, const FString& NameForErrors, FSUDSMessageLogger& Logger);
	static bool WriteBackGosubID(const FString& GosubID, int LineNo, TArray<FString>& Lines, const FString& NameForErrors, FSUDSMessageLogger& Logger);
	static bool TextIDCheckMatch(const FText& AssetText, const FString& SourceLine);

	
};
