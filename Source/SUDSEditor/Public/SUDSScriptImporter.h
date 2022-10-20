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
	/// Parse a single line
	void ParseLine(const FStringView& Line);

};
