// Copyright Steve Streeting 2022
// Released under the MIT license https://opensource.org/license/MIT/
#pragma once
#include "CoreMinimal.h"

// Use DECLARE_LOG_CATEGORY_CLASS not DECLARE_LOG_CATEGORY_EXTERN because we use UE_LOG in headers
DECLARE_LOG_CATEGORY_CLASS(LogSUDS, Warning, All)

#define SUDS_RANDOMITEM_VAR "SUDS.RandomItem"

struct FSUDSConstants
{
	/// Reserved variable named use to create random results from select nodes
	static const FName RandomItemSelectIndexVarName;

};