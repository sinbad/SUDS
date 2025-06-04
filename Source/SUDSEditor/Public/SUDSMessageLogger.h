// Copyright Steve Streeting 2022
// Released under the MIT license https://opensource.org/license/MIT/
#pragma once

#include "CoreMinimal.h"
#include "Logging/TokenizedMessage.h"

class FTokenizedMessage;

struct SUDSEDITOR_API FSUDSMessageLogger
{
protected:
	TArray<TSharedRef<FTokenizedMessage>> ErrorMessages;

	bool bWriteToMessageLog = true;
public:
	FSUDSMessageLogger() {}
	FSUDSMessageLogger(bool bWriteToMsg) : bWriteToMessageLog(bWriteToMsg) {}
	~FSUDSMessageLogger();

	void SetWriteToMessageLog(bool bWrite) { bWriteToMessageLog = bWrite; }
	bool HasErrors() const;
	int NumErrors() const;
	
	bool GetWriteToMessageLog() const { return bWriteToMessageLog; }
	void AddMessage(EMessageSeverity::Type Severity, const FText& Text);
	
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 6 
	template <typename... Types>
	FORCEINLINE void Logf(ELogVerbosity::Type Verbosity, UE::Core::TCheckedFormatString<FString::FmtCharType, Types...> Fmt, Types... Args)
#else 
	template <typename FmtType, typename... Types>
	FORCEINLINE void Logf(ELogVerbosity::Type Verbosity, const FmtType& Fmt, Types... Args)
#endif
	{
		EMessageSeverity::Type Sev = EMessageSeverity::Info;
		switch(Verbosity)
		{
		case ELogVerbosity::Fatal:
		case ELogVerbosity::Error:
			Sev = EMessageSeverity::Error;
			break;
		case ELogVerbosity::Warning:
			Sev = EMessageSeverity::Warning;
			break;
		default: ;
		}
		AddMessage(Sev, FText::FromString(FString::Printf(Fmt, Args...)));
	}

	const TArray<TSharedRef<FTokenizedMessage>>& GetErrorMessages() const { return ErrorMessages; }

	/// Clear messages in preparation for an import
	static void ClearMessages();

	

};
