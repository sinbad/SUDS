#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SUDSSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSUDSSubsystem, Log, All);
/**
 * 
 */
UCLASS()
class SUDS_API USUDSSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

};

inline USUDSSubsystem* GetSUDSSubsystem(UWorld* WorldContext)
{
	if (IsValid(WorldContext) && WorldContext->IsGameWorld())
	{
		auto GI = WorldContext->GetGameInstance();
		if (IsValid(GI))
			return GI->GetSubsystem<USUDSSubsystem>();		
	}
		
	return nullptr;
}
