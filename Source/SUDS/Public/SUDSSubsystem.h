#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SUDSSubsystem.generated.h"

class USUDSDialogue;
class USUDSScript;
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

	
	/**
	 * Create a dialogue instance based on a script.
	 * @param Owner The owner of this instance. Can be any object but determines the lifespan of this dialogue,
	 *   could make sense to make the owner the NPC you're talking to for example.
	 * @param Script The script to base this dialogue on
	 * @param StartAtLabel The label to start at. If none, start at the beginning.
	 * @return 
	 */
	UFUNCTION(BlueprintCallable)
	USUDSDialogue* CreateDialogue(UObject* Owner, USUDSScript* Script, FName StartAtLabel = NAME_None);

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
