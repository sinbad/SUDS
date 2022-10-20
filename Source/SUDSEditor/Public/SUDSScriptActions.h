#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

/**
 * 
 */
class FSUDSScriptActions : public FAssetTypeActions_Base
{
public:
	virtual FText GetName() const override;
	virtual FString GetObjectDisplayName(UObject* Object) const override;
	virtual UClass* GetSupportedClass() const override;
	virtual FColor GetTypeColor() const override;
	virtual uint32 GetCategories() override;
	virtual void GetResolvedSourceFilePaths(const TArray<UObject*>& TypeAssets,
		TArray<FString>& OutSourceFilePaths) const override;
	virtual bool IsImportedAsset() const override { return true; }
};
