#include "SUDSScriptActions.h"

#include "IMessageLogListing.h"
#include "MessageLogModule.h"
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


struct FSUDSMessageLogger
{
protected:
	TArray<TSharedRef<FTokenizedMessage>> ErrorMessages;

public:
	FSUDSMessageLogger() {}
	~FSUDSMessageLogger()
	{
		//Always clear the old message after an import or re-import
		const TCHAR* LogTitle = TEXT("SUDS");
		FMessageLogModule& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");
		TSharedPtr<class IMessageLogListing> LogListing = MessageLogModule.GetLogListing(LogTitle);
		LogListing->SetLabel(FText::FromString("SUDS"));
		LogListing->ClearMessages();

		if(ErrorMessages.Num() > 0)
		{
			LogListing->AddMessages(ErrorMessages);
			MessageLogModule.OpenMessageLog(LogTitle);
		}
	}

	bool HasErrors() const
	{
		for (const TSharedRef<FTokenizedMessage> Msg : ErrorMessages)
		{
			if (Msg->GetSeverity() == EMessageSeverity::CriticalError || Msg->GetSeverity() == EMessageSeverity::Error)
			{
				return true;
			}
		}
		return false;		
	}

	int NumErrors() const
	{
		int Errs = 0;
		for (const TSharedRef<FTokenizedMessage> Msg : ErrorMessages)
		{
			if (Msg->GetSeverity() == EMessageSeverity::CriticalError || Msg->GetSeverity() == EMessageSeverity::Error)
			{
				++Errs;
			}
		}
		return Errs;
	}
	
	
	void AddMessage(EMessageSeverity::Type Severity, const FText& Text)
	{
		ErrorMessages.Add(FTokenizedMessage::Create(Severity, Text));
	}


};


void FSUDSScriptActions::WriteBackTextIDs(TArray<TWeakObjectPtr<USUDSScript>> Scripts)
{
	FSUDSMessageLogger Logger;
	for (auto Script : Scripts)
	{
		if (Script.IsValid())
		{
			WriteBackTextIDs(Script.Get(), Logger);
		}
	}
}

void FSUDSScriptActions::WriteBackTextIDs(USUDSScript* Script, FSUDSMessageLogger& Logger)
{
	const auto& SrcData = Script->AssetImportData->SourceData; 
	if (SrcData.SourceFiles.Num() == 1)
	{
		TArray<FString> Lines;
		FString SourceFile =  SrcData.SourceFiles[0].RelativeFilename;
		auto Package = Script->GetPackage();
		if (FPaths::IsRelative(SourceFile) && Package)
		{
			FString PackagePath = FPackageName::LongPackageNameToFilename(FPackageName::GetLongPackagePath(Package->GetPathName()));
			SourceFile = FPaths::ConvertRelativePathToFull(PackagePath, SourceFile);
		}
		if (FFileHelper::LoadFileToStringArray(Lines, *SourceFile))
		{
			const int PrevErrs = Logger.NumErrors();
			const bool bHeaderChanges = WriteBackTextIDsFromNodes(Script->GetHeaderNodes(), Lines, Script->GetName(), Logger);
			const bool bBodyChanges = WriteBackTextIDsFromNodes(Script->GetNodes(), Lines, Script->GetName(), Logger);

			if (Logger.NumErrors() > PrevErrs)
			{
				Logger.AddMessage(EMessageSeverity::Info,
								  FText::FromString(FString::Printf(TEXT("Errors prevented saving updates to %s."), *Script->GetName())));
			}
			else if (bHeaderChanges || bBodyChanges)
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

				// Write source file back; always encode as UTF8 not default ANSI/UTF16
				// Do this before updating asset import data so it picks up new file timestamp
				FFileHelper::SaveStringToFile(FStringView(CombinedString), *SourceFile, FFileHelper::EEncodingOptions::ForceUTF8);

				/*  BEGIN try to prevent re-import prompt
				 *  This unfortunately didn't work, so removed & let it reimport
				 
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

				// Even this doesn't work:
				GUnrealEd->AutoReimportManager->IgnoreFileModification(SourceFile);

				* END try to prevent re-import
				*/

			}
			else
			{
				Logger.AddMessage(EMessageSeverity::Info,
								  FText::FromString(FString::Printf(TEXT("No changes were required to %s"), *Script->GetName())));
			}

		}
		else
		{
			Logger.AddMessage(EMessageSeverity::Error,
			                  FText::FromString(FString::Printf(TEXT("Error opening source asset %s"), *SourceFile)));
		}
	}
	else
	{
		Logger.AddMessage(EMessageSeverity::Error,
		                  FText::FromString(
			                  FString::Printf(TEXT("No source files associated with asset %s"), *Script->GetName())));
	}
}

bool FSUDSScriptActions::WriteBackTextIDsFromNodes(const TArray<USUDSScriptNode*> Nodes, TArray<FString>& Lines, const FString& NameForErrors, FSUDSMessageLogger& Logger)
{
	bool bAnyChanges = false;
	// For each speaker line, set line and choice edge, use source line no to append text ID
	for (const auto N : Nodes)
	{
		if (N->GetNodeType() == ESUDSScriptNodeType::Text)
		{
			if (const auto* TN = Cast<USUDSScriptNodeText>(N))
			{
				bAnyChanges = WriteBackTextID(TN->GetText(), TN->GetSourceLineNo(), Lines, NameForErrors, Logger) || bAnyChanges;
			}
		}
		else if (N->GetNodeType() == ESUDSScriptNodeType::SetVariable)
		{
			if (const auto* SN = Cast<USUDSScriptNodeSet>(N))
			{
				if (SN->GetExpression().IsTextLiteral())
				{
					FText Literal = SN->GetExpression().GetTextLiteralValue();
					
					bAnyChanges = WriteBackTextID(Literal, SN->GetSourceLineNo(), Lines, NameForErrors, Logger) || bAnyChanges;
				}
			}
			
		}
		else if (N->GetNodeType() == ESUDSScriptNodeType::Choice)
		{
			// Edges
			for (auto& Edge : N->GetEdges())
			{
				bAnyChanges = WriteBackTextID(Edge.GetText(), Edge.GetSourceLineNo(), Lines, NameForErrors, Logger) || bAnyChanges;
			}
		}
	}

	return bAnyChanges;
}

bool FSUDSScriptActions::WriteBackTextID(const FText& AssetText, int LineNo, TArray<FString>& Lines, const FString& NameForErrors, FSUDSMessageLogger& Logger)
{
	if (AssetText.IsEmpty())
		return false;

	// Line numbers are 1-based
	const int Idx = LineNo - 1;

	if (!Lines.IsValidIndex(Idx))
	{
		Logger.AddMessage(EMessageSeverity::Error,
		                  FText::FromString(FString::Printf(
			                  TEXT("Cannot write back TextID to '%s', source line number %d is invalid (Text: '%s')"),
			                  *NameForErrors,
			                  LineNo,
			                  *AssetText.ToString())));
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
				Lines[Idx] = UpdatedLine;
				return true;
			}
			else
			{
				Logger.AddMessage(EMessageSeverity::Error,
				                  FText::FromString(FString::Printf(TEXT(
					                  "Tried to update TextID on line %d of %s but source file did not contain expected text '%s'"
				                  ),
				                                                    LineNo,
				                                                    *NameForErrors,
				                                                    *AssetText.ToString())));
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
			FString UpdatedLine = FString::Printf(TEXT("%s    %s"), *SourceLine, *TextID); 
			Lines[Idx] = UpdatedLine;
			return true;
		}
		else
		{
			Logger.AddMessage(EMessageSeverity::Error,
			                  FText::FromString(FString::Printf(TEXT(
				                  "Tried to write TextID back to line %d of %s but source file did not contain expected text '%s'"
			                  ),
			                                                    LineNo,
			                                                    *NameForErrors,
			                                                    *AssetText.ToString())));
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
