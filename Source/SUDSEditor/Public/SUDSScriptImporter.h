#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
class SUDSEDITOR_API FSUDSScriptImporter
{
public:
	bool ImportFromBuffer(const TCHAR* Buffer, int32 Len, const FString& NameForErrors, bool bSilent);

protected:
	const int TabIndentValue = 4;
	bool bHeaderDone = false;
	bool bHeaderInProgress = false;
	bool bTextInProgress = false;
	/// Parse a single line
	bool ParseLine(const FStringView& Line, int LineNo, const FString& NameForErrors, bool bSilent);
	bool ParseHeaderLine(const FStringView& Line, int LineNo, const FString& NameForErrors, bool bSilent);
	FStringView TrimLine(const FStringView& Line, int* OutIndentLevel) const;

};
