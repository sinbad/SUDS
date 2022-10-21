#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SUDSScript.generated.h"

/**
 * A single SUDS script asset.
 */
UCLASS(BlueprintType)
class SUDS_API USUDSScript : public UObject
{
	GENERATED_BODY()

public:

#if WITH_EDITORONLY_DATA
	// Import data for this 
	UPROPERTY(VisibleAnywhere, Instanced, Category=ImportSettings)
	TObjectPtr<class UAssetImportData> AssetImportData;

	// Array of nodes (static after import)
	
	// UObject interface
	virtual void PostInitProperties() override;
	virtual void GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const override;
	virtual void Serialize(FArchive& Ar) override;
	// End of UObject interface
#endif
	
};
