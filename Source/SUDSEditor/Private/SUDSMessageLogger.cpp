// Copyright Steve Streeting 2022
// Released under the MIT license https://opensource.org/license/MIT/
#include "SUDSMessageLogger.h"
#include "IMessageLogListing.h"
#include "MessageLogModule.h"

FSUDSMessageLogger::~FSUDSMessageLogger()
{
	if (bWriteToMessageLog)
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
}

bool FSUDSMessageLogger::HasErrors() const
{
	for (const TSharedRef<FTokenizedMessage>& Msg : ErrorMessages)
	{
		if (Msg->GetSeverity() == EMessageSeverity::Error)
		{
			return true;
		}
	}
	return false;		
}

int FSUDSMessageLogger::NumErrors() const
{
	int Errs = 0;
	for (const TSharedRef<FTokenizedMessage>& Msg : ErrorMessages)
	{
		if (Msg->GetSeverity() == EMessageSeverity::Error)
		{
			++Errs;
		}
	}
	return Errs;
}

void FSUDSMessageLogger::AddMessage(EMessageSeverity::Type Severity, const FText& Text)
{
	ErrorMessages.Add(FTokenizedMessage::Create(Severity, Text));
}

