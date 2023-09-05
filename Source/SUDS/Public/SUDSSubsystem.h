// Copyright Steve Streeting 2022
// Released under the MIT license https://opensource.org/license/MIT/
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "SUDSSubsystem.generated.h"

class USUDSDialogue;
class USUDSScript;
class USoundConcurrency;
struct FSoundConcurrencySettings;
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

protected:
	UPROPERTY()
	USoundConcurrency* VoiceConcurrency;

public:
	/**
	 * Sets the number of voiced lines that can be played at once. Defaults to 1, so that when a new voiced line is
	 * played while another is still playing, the previous one is stopped. If you would prefer that they overlap, set
	 * this to >1
	 * @param ConcurrentLines The number of voice lines that can be played at once. 
	 */
	UFUNCTION(BlueprintCallable, Category="SUDS|Settings")
	void SetMaxConcurrentVoicedLines(int ConcurrentLines);

	/**
	 * Gets the number of voiced lines that can be played at once. Defaults to 1, so that when a new voiced line is
	 * played while another is still playing, the previous one is stopped. 
	 */
	UFUNCTION(BlueprintCallable, Category="SUDS|Settings")
	int GetMaxConcurrentVoicedLines() const;
	/**
	 * Sets all the concurrency settings for voiced lines.
	 * @param NewSettings 
	 */
	UFUNCTION(BlueprintCallable, Category="SUDS|Settings")
	void SetVoicedLineConcurrencySettings(const FSoundConcurrencySettings& NewSettings);


	/// Get the concurrency settings for voiced lines
	UFUNCTION(BlueprintCallable, Category="SUDS|Settings")
	const FSoundConcurrencySettings& GetVoicedLineConcurrencySettings() const;

	USoundConcurrency* GetVoicedLineConcurrency() const { return VoiceConcurrency; }
	
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
