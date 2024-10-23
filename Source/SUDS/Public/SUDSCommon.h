// Copyright Steve Streeting 2022
// Released under the MIT license https://opensource.org/license/MIT/
#pragma once
#include "CoreMinimal.h"
#include "Runtime/Launch/Resources/Version.h"

// Use DECLARE_LOG_CATEGORY_CLASS not DECLARE_LOG_CATEGORY_EXTERN because we use UE_LOG in headers
DECLARE_LOG_CATEGORY_CLASS(LogSUDS, Warning, All)

#define SUDS_RANDOMITEM_VAR "SUDS.RandomItem"

struct FSUDSConstants
{
	/// Reserved variable named use to create random results from select nodes
	static const FName RandomItemSelectIndexVarName;

};

#if ENGINE_MINOR_VERSION >= 5
#define SUDS_GET_TEXT_KEY(Text) FTextInspector::GetTextId(Text).GetKey().ToString()
#else
#define SUDS_GET_TEXT_KEY(Text) FTextInspector::GetTextId(Text).GetKey().GetChars()
#endif