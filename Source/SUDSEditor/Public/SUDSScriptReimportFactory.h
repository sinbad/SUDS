// Copyright Steve Streeting 2022
// Released under the MIT license https://opensource.org/license/MIT/
#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "EditorReimportHandler.h"
#include "SUDSScriptFactory.h"

#include "SUDSScriptReimportFactory.generated.h"

// Reimports a USUDSScriptFactory asset
// Necessary to get the import pop-up 
UCLASS()
class USUDSScriptReimportFactory : public USUDSScriptFactory, public FReimportHandler
{
	GENERATED_BODY()
public:
	USUDSScriptReimportFactory();
	
	//~ Begin FReimportHandler Interface
	virtual bool CanReimport(UObject* Obj, TArray<FString>& OutFilenames) override;
	virtual void SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths) override;
	virtual EReimportResult::Type Reimport(UObject* Obj) override;
	virtual int32 GetPriority() const override;
	//~ End FReimportHandler Interface
};