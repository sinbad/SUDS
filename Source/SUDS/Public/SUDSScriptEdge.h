#pragma once

#include "CoreMinimal.h"
#include "SUDSScriptEdge.generated.h"

/**
* Edge in the script graph. An edge leads to another node (unidirectional) by index. 
*/
USTRUCT()
struct SUDS_API FSUDSScriptEdge
{
	GENERATED_BODY()
public:
	UPROPERTY()
	int32 TargetNodeIndex;

	// TODO Add conditions
};