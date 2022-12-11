#include "SUDSScriptActions.h"

#include "SUDSEditor.h"
#include "SUDSScript.h"
#include "SUDSScriptImporter.h"
#include "SUDSScriptNode.h"
#include "SUDSScriptNodeSet.h"
#include "SUDSScriptNodeText.h"
#include "ToolMenuSection.h"
#include "EditorFramework/AssetImportData.h"
#include "Misc/FileHelper.h"

FText FSUDSScriptActions::GetName() const
{
	return INVTEXT("SUDS Script");
}

FString FSUDSScriptActions::GetObjectDisplayName(UObject* Object) const
{
	return Object->GetName();
}

UClass* FSUDSScriptActions::GetSupportedClass() const
{
	return USUDSScript::StaticClass();
}

FColor FSUDSScriptActions::GetTypeColor() const
{
	return FColor::Orange;
}

uint32 FSUDSScriptActions::GetCategories()
{
	return EAssetTypeCategories::Misc;
}

void FSUDSScriptActions::GetResolvedSourceFilePaths(const TArray<UObject*>& TypeAssets,
	TArray<FString>& OutSourceFilePaths) const
{
	for (auto& Asset : TypeAssets)
	{
		const auto Script = CastChecked<USUDSScript>(Asset);
		if (Script->AssetImportData)
		{
			Script->AssetImportData->ExtractFilenames(OutSourceFilePaths);
		}
	}
}

bool FSUDSScriptActions::HasActions(const TArray<UObject*>& InObjects) const
{
	return true;
}

void FSUDSScriptActions::GetActions(const TArray<UObject*>& InObjects, FToolMenuSection& Section)
{
	const auto Scripts = GetTypedWeakObjectPtrs<USUDSScript>(InObjects);

	Section.AddMenuEntry(
		"WriteBackTextIDs",
		NSLOCTEXT("SUDS", "WriteBackTextIDs", "Write Back String Keys"),
		NSLOCTEXT("SUDS",
		          "WriteBackTextIDsTooltip",
		          "Write string table keys back to source script to make them constant for localisation."),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Edit"),
		FUIAction(
			FExecuteAction::CreateSP(this, &FSUDSScriptActions::WriteBackTextIDs, Scripts),
			FCanExecuteAction()
		)
	);
}


void FSUDSScriptActions::WriteBackTextIDs(TArray<TWeakObjectPtr<USUDSScript>> Scripts)
{
	for (auto Script : Scripts)
	{
		if (Script.IsValid())
		{
			WriteBackTextIDs(Script.Get());
		}
	}
}

void FSUDSScriptActions::WriteBackTextIDs(USUDSScript* Script)
{
	const auto& SrcData = Script->AssetImportData->SourceData; 
	if (SrcData.SourceFiles.Num() == 1)
	{
		TArray<FString> Lines;
		const FString SourceFile = SrcData.SourceFiles[0].RelativeFilename;
		if (FFileHelper::LoadFileToStringArray(Lines, *SourceFile))
		{
			const bool bHeaderChanges = WriteBackTextIDsFromNodes(Script->GetHeaderNodes(), Lines, Script->GetName());
			const bool bBodyChanges = WriteBackTextIDsFromNodes(Script->GetNodes(), Lines, Script->GetName());

			if (bHeaderChanges || bBodyChanges)
			{
				// We need to re-hash file in memory before saving to prevent re-import
				int32 Length = 10;
				for(const FString& Line : Lines)
				{
					Length += Line.Len() + UE_ARRAY_COUNT(LINE_TERMINATOR);
				}
				FString CombinedString;
				CombinedString.Reserve(Length);

				for(const FString& Line : Lines)
				{
					CombinedString += Line;
					CombinedString += LINE_TERMINATOR;
				}
				
				const FMD5Hash FileHash = FSUDSScriptImporter::CalculateHash(*CombinedString, CombinedString.Len());
				Script->AssetImportData->Update(SourceFile, FileHash);

				// Need to mark asset dirty since hash has changed
				// Try to find the outer package so we can dirty it up
				if (Script->GetOuter())
				{
					Script->GetOuter()->MarkPackageDirty();
				}
				else
				{
					Script->MarkPackageDirty();
				}

				// Write source file back; always encode as UTF8 not default ANSI/UTF16
				FFileHelper::SaveStringToFile(FStringView(CombinedString), *SourceFile, FFileHelper::EEncodingOptions::ForceUTF8);
			}
			else
			{
				// TODO: Print no changes needed
			}

		}
		else
		{
			UE_LOG(LogSUDSEditor, Error, TEXT("Error opening source asset %s"), *SourceFile);
		}
	}
	else
	{
		UE_LOG(LogSUDSEditor, Error, TEXT("No source files associated with asset %s"), *Script->GetName());
	}
}

bool FSUDSScriptActions::WriteBackTextIDsFromNodes(const TArray<USUDSScriptNode*> Nodes, TArray<FString>& Lines, const FString& NameForErrors)
{
	bool bAnyChanges = false;
	// For each speaker line, set line and choice edge, use source line no to append text ID
	for (const auto N : Nodes)
	{
		if (N->GetNodeType() == ESUDSScriptNodeType::Text)
		{
			if (const auto* TN = Cast<USUDSScriptNodeText>(N))
			{
				bAnyChanges = WriteBackTextID(TN->GetText(), TN->GetSourceLineNo(), Lines, NameForErrors) || bAnyChanges;
			}
		}
		else if (N->GetNodeType() == ESUDSScriptNodeType::SetVariable)
		{
			if (const auto* SN = Cast<USUDSScriptNodeSet>(N))
			{
				if (SN->GetExpression().IsTextLiteral())
				{
					FText Literal = SN->GetExpression().GetTextLiteralValue();
					
					bAnyChanges = WriteBackTextID(Literal, SN->GetSourceLineNo(), Lines, NameForErrors) || bAnyChanges;
				}
			}
			
		}
		else if (N->GetNodeType() == ESUDSScriptNodeType::Choice)
		{
			// Edges
			for (auto& Edge : N->GetEdges())
			{
				bAnyChanges = WriteBackTextID(Edge.GetText(), Edge.GetSourceLineNo(), Lines, NameForErrors) || bAnyChanges;
			}
		}
	}

	return bAnyChanges;
}

bool FSUDSScriptActions::WriteBackTextID(const FText& AssetText, int LineNo, TArray<FString>& Lines, const FString& NameForErrors)
{
	if (AssetText.IsEmpty())
		return false;

	// Line numbers are 1-based
	const int Idx = LineNo - 1;

	if (!Lines.IsValidIndex(Idx))
	{
		UE_LOG(LogSUDSEditor,
		       Error,
		       TEXT("Cannot write back TextID to '%s', source line number %d is invalid (Text: '%s')"),
		       *NameForErrors,
		       LineNo,
		       *AssetText.ToString());
		return false;
	}

	const FString& SourceLine = Lines[Idx];
	FString ExistingTextID;
	int ExistingNum;
	FStringView SourceLineView(SourceLine);
	if (FSUDSScriptImporter::RetrieveTextIDFromLine(SourceLineView, ExistingTextID, ExistingNum))
	{
		// Existing TextID - replace if already there
		const FString TextID = FTextInspector::GetTextId(AssetText).GetKey().GetChars();
		if (ExistingTextID != TextID)
		{
			if (TextIDCheckMatch(AssetText, SourceLine))
			{
				FString Prefix(SourceLineView);
				FString UpdatedLine = FString::Printf(TEXT("%s    %s"), *Prefix, *TextID); 
				Lines[LineNo] = UpdatedLine;
				return true;
			}
			else
			{
				UE_LOG(LogSUDSEditor,
					   Error,
					   TEXT("Tried to update TextID on line %d of %s but source file did not contain expected text '%s'"
					   ),
					   LineNo,
					   *NameForErrors,
					   *AssetText.ToString());
				return false;
				
			}
		}
		// Same, no need to change
		return false;
		
	}
	else
	{
		if (TextIDCheckMatch(AssetText, SourceLine))
		{
			const FString TextID = FTextInspector::GetTextId(AssetText).GetKey().GetChars();
			FString UpdatedLine = FString::Printf(TEXT("%s    %s"), *(Lines[LineNo]), *TextID); 
			Lines[LineNo] = UpdatedLine;
			return true;
		}
		else
		{
			UE_LOG(LogSUDSEditor,
			       Error,
			       TEXT("Tried to write TextID back to line %d of %s but source file did not contain expected text '%s'"
			       ),
			       LineNo,
			       *NameForErrors,
			       *AssetText.ToString());
			return false;
		}
	}
}

bool FSUDSScriptActions::TextIDCheckMatch(const FText& AssetText, const FString& SourceLine)
{
	// Text from our asset must be present in the text in the source line
	// However, in case this is a multi-line string, take the first line only
	FString AssetStr = AssetText.ToString();
	{
		FString L, R;
		if (AssetStr.Split("\n", &L, &R))
		{
			AssetStr = L.TrimStartAndEnd();
		}
	}

	// Bear in mind that this line might be a text line, a choice or a set line
	// therefore we only check if the source line contains the asset text

	return SourceLine.Contains(AssetStr);
	
}
