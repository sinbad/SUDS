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
			WriteBackTextIDsFromNodes(Script->GetHeaderNodes(), Lines);
			WriteBackTextIDsFromNodes(Script->GetNodes(), Lines);
			
			// Write data to new file then close
			const FString DestFile = FPaths::CreateTempFilename(*FPaths::ProjectSavedDir());
			FFileHelper::SaveStringArrayToFile(Lines, *DestFile);
			// BEFORE flipping over the files, we'll need to update the file hash
			const auto FileHash = FMD5Hash::HashFile(*DestFile);
			Script->AssetImportData->Update(SourceFile, FileHash);

			// TODO:
			//   1. Perform source control checkout of source file
			//   2. Flip orig source to a temp old file
			//   3. Flip temp dest to source
			//   4. Delete temp old
		}
		else
		{
			UE_LOG(LogSUDSEditor, Error, TEXT("Error opening source asset %s"), *Script->GetName());
			
		}
	}
	else
	{
		UE_LOG(LogSUDSEditor, Error, TEXT("No source files associated with asset %s"), *Script->GetName());
	}
}

void FSUDSScriptActions::WriteBackTextIDsFromNodes(const TArray<USUDSScriptNode*> Nodes, TArray<FString>& Lines)
{
	// For each speaker line, set line and choice edge, use source line no to append text ID
	for (const auto N : Nodes)
	{
		if (N->GetNodeType() == ESUDSScriptNodeType::Text)
		{
			if (const auto* TN = Cast<USUDSScriptNodeText>(N))
			{
				WriteBackTextID(TN->GetText(), TN->GetSourceLineNo(), Lines);
			}
		}
		else if (N->GetNodeType() == ESUDSScriptNodeType::SetVariable)
		{
			if (const auto* SN = Cast<USUDSScriptNodeSet>(N))
			{
				if (SN->GetExpression().IsTextLiteral())
				{
					FText Literal = SN->GetExpression().GetTextLiteralValue();
					
					WriteBackTextID(Literal, SN->GetSourceLineNo(), Lines);
				}
			}
			
		}
		else if (N->GetNodeType() == ESUDSScriptNodeType::Choice)
		{
			// Edges
			for (auto& Edge : N->GetEdges())
			{
				WriteBackTextID(Edge.GetText(), Edge.GetSourceLineNo(), Lines);
			}
		}
	}
	
}

void FSUDSScriptActions::WriteBackTextID(const FText& Text, int LineNo, TArray<FString>& Lines)
{
	if (Text.IsEmpty())
		return;

	if (!Lines.IsValidIndex(LineNo))
	{
		UE_LOG(LogSUDSEditor, Error, TEXT("Cannot write back TextID for '%s', source line number %d is invalid"), *Text.ToString(), LineNo);
		return;
	}

	const FString& SourceLine = Lines[LineNo];
	FString LineWithout;
	FString ExistingTextID;
	int ExistingNum;
	if (FSUDSScriptImporter::RetrieveTextIDFromLine(SourceLine, ExistingTextID, LineWithout, ExistingNum))
	{
		// TODO: Existing TextID - replace if already there and warn if not the same?
		
	}
	else
	{
		// Text from our asset must start with the text in the line
		// StartWith with because source might be multiple lines
		if (!Text.ToString().StartsWith(Lines[LineNo]))
		{
			return;
		}
		FString TextID = FTextInspector::GetTextId(Text).GetKey().GetChars();
		FString UpdatedLine = FString::Printf(TEXT("%s    %s"), *(Lines[LineNo]), *TextID); 
		Lines[LineNo] = UpdatedLine;
	}
}
