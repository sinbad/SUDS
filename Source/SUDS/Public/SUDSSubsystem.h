// Copyright Steve Streeting 2022
// Released under the MIT license https://opensource.org/license/MIT/
#pragma once

#include "CoreMinimal.h"
#include "SUDSValue.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "SUDSSubsystem.generated.h"

class USUDSDialogue;
class USUDSScript;
class USoundConcurrency;
struct FSoundConcurrencySettings;
DECLARE_LOG_CATEGORY_EXTERN(LogSUDSSubsystem, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnGlobalVariableChangedEvent, FName, VariableName, const FSUDSValue&, Value, bool, bFromScript);

/// Copy of the global state of the system
USTRUCT(BlueprintType)
struct FSUDSGlobalState
{
	GENERATED_BODY()
protected:
	UPROPERTY(BlueprintReadOnly, SaveGame, Category="SUDS")
	TMap<FName, FSUDSValue> GlobalVariables;

public:
	FSUDSGlobalState() {}

	FSUDSGlobalState(const TMap<FName, FSUDSValue>& InGlobalVars) : GlobalVariables(InGlobalVars)
	{
	}

	const TMap<FName, FSUDSValue>& GetGlobalVariables() const { return GlobalVariables; }

	SUDS_API friend FArchive& operator<<(FArchive& Ar, FSUDSGlobalState& Value);
	SUDS_API friend void operator<<(FStructuredArchive::FSlot Slot, FSUDSGlobalState& Value);
	bool Serialize(FStructuredArchive::FSlot Slot)
	{
		Slot << *this;
		return true;
	}
	bool Serialize(FArchive& Ar)
	{
		Ar << *this;
		return true;
	}
	
};

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
	
	/// Event raised when a global variable is changed. "FromScript" is true if the variable was set by the script, false if set from code
	UPROPERTY(BlueprintAssignable)
	FOnGlobalVariableChangedEvent OnGlobalVariableChanged;

protected:
	UPROPERTY()
	USoundConcurrency* VoiceConcurrency;
	
	/// Global variable state
	TMap<FName, FSUDSValue> GlobalVariableState;
	
	void SetGlobalVariableImpl(FName Name, const FSUDSValue& Value, bool bFromScript, int LineNo)
	{
		const FSUDSValue OldValue = GetGlobalVariable(Name);
		if (!IsGlobalVariableSet(Name) ||
			(OldValue != Value).GetBooleanValue())
		{
			GlobalVariableState.Add(Name, Value);
			OnGlobalVariableChanged.Broadcast(Name, Value, bFromScript);
		}
	}	

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


	/**
	 * Reset the global state of the system.
	 * @param bResetVariables If true, resets all variable state
	 */
	UFUNCTION(BlueprintCallable, Category="SUDS|Global State")
	void ResetGlobalState(bool bResetVariables = true);

	/** Retrieve a copy of the global state of the system.
	 *  This is useful for saving the state of e.g. global variables.
	 *  @return A static copy of the current global state. This struct can be serialised with your save data,
	 *  and contains the state of global variables.
	 */
	UFUNCTION(BlueprintCallable, Category="SUDS|Global State")
	FSUDSGlobalState GetSavedGlobalState() const;

	/** Restore the global saved state.
	 *  This is useful for restoring global state eg global variables when you're loading a game.
	 *  @param State Global state that you previously retrieved from GetSavedGlobalState().
	 */
	UFUNCTION(BlueprintCallable, Category="SUDS|Dialogue")
	void RestoreSavedGlobalState(const FSUDSGlobalState& State);
	
	/// Set a global variable
	/// This is mostly only useful if you happen to already have a general purpose FSUDSValue.
	/// See SetGlobalVariableText, SetGlobalVariableInt etc for literal-friendly versions
	UFUNCTION(BlueprintCallable, Category="SUDS|Global Variables")
	void SetGlobalVariable(FName Name, FSUDSValue Value)
	{
		SetGlobalVariableImpl(Name, Value, false, 0);
	}

	/// Internal use only
	void InternalSetGlobalVariable(FName Name, const FSUDSValue& Value, bool bFromScript, int LineNo) { SetGlobalVariableImpl(Name, Value, bFromScript, LineNo); }

	/// Get a variable in dialogue state as a general value type
	/// See GetDialogueText, GetDialogueInt etc for more type friendly versions, but if you want to access the state
	/// as a type-flexible value then you can do so with this function.
	UFUNCTION(BlueprintCallable, Category="SUDS|Global Variables")
	FSUDSValue GetGlobalVariable(FName Name) const
	{
		if (const auto Arg = GlobalVariableState.Find(Name))
		{
			return *Arg;
		}
		return FSUDSValue();
	}

	UFUNCTION(BlueprintCallable, Category="SUDS|Global Variables")
	bool IsGlobalVariableSet(FName Name) const
	{
		return GlobalVariableState.Contains(Name);
	}

	/// Get all variables
	UFUNCTION(BlueprintCallable, Category="SUDS|Global Variables")
	const TMap<FName, FSUDSValue>& GetGlobalVariables() const { return GlobalVariableState; }
	
	/**
	 * Set a text global variable
	 * @param Name The name of the variable
	 * @param Value The value of the variable
	 */
	UFUNCTION(BlueprintCallable, Category="SUDS|Global Variables")
	void SetGlobalVariableText(FName Name, FText Value)
	{
		SetGlobalVariable(Name, Value);
	}

	/**
	 * Get a text global variable
	 * @param Name The name of the variable
	 * @returns Value The value of the variable
	 */
	UFUNCTION(BlueprintCallable, Category="SUDS|Global Variables")
	FText GetGlobalVariableText(FName Name) const;

	/**
	 * Set a global variable on the passed in parameters collection.
	 * @param Name The name of the variable
	 * @param Value The value of the variable
	 */
	UFUNCTION(BlueprintCallable, Category="SUDS|Global Variables")
	void SetGlobalVariableInt(FName Name, int32 Value);

	/**
	 * Get an int global variable
	 * @param Name The name of the variable
	 * @returns Value The value of the variable
	 */
	UFUNCTION(BlueprintCallable, Category="SUDS|Global Variables")
	int GetGlobalVariableInt(FName Name) const;
	
	/**
	 * Set a float global variable 
	 * @param Name The name of the variable
	 * @param Value The value of the variable
	 */
	UFUNCTION(BlueprintCallable, Category="SUDS|Global Variables")
	void SetGlobalVariableFloat(FName Name, float Value);

	/**
	 * Get a float global variable
	 * @param Name The name of the variable
	 * @returns Value The value of the variable
	 */
	UFUNCTION(BlueprintCallable, Category="SUDS|Global Variables")
	float GetGlobalVariableFloat(FName Name) const;
	
	/**
	 * Set a gender global variable 
	 * @param Name The name of the variable
	 * @param Value The value of the variable
	 */
	UFUNCTION(BlueprintCallable, Category="SUDS|Global Variables")
	void SetGlobalVariableGender(FName Name, ETextGender Value);

	/**
	 * Get a gender global variable
	 * @param Name The name of the variable
	 * @returns Value The value of the variable
	 */
	UFUNCTION(BlueprintCallable, Category="SUDS|Global Variables")
	ETextGender GetGlobalVariableGender(FName Name) const;

	/**
	 * Set a boolean global variable 
	 * @param Name The name of the variable
	 * @param Value The value of the variable
	 */
	UFUNCTION(BlueprintCallable, Category="SUDS|Global Variables")
	void SetGlobalVariableBoolean(FName Name, bool Value);

	/**
	 * Get a boolean global variable
	 * @param Name The name of the variable
	 * @returns Value The value of the variable
	 */
	UFUNCTION(BlueprintCallable, Category="SUDS|Global Variables")
	bool GetGlobalVariableBoolean(FName Name) const;

	/**
	 * Set a name global variable
	 * @param Name The name of the variable
	 * @param Value The value of the variable
	 */
	UFUNCTION(BlueprintCallable, Category="SUDS|Global Variables")
	void SetGlobalVariableName(FName Name, FName Value);

	/**
	 * Get a name global variable
	 * @param Name The name of the variable
	 * @returns Value The value of the variable
	 */
	UFUNCTION(BlueprintCallable, Category="SUDS|Global Variables")
	FName GetGlobalVariableName(FName Name) const;

	
	/**
	 * Remove the definition of a global variable.
	 * This has much same effect as setting the variable back to the default value for this type, since attempting to
	 * retrieve a missing variable result in a default value.
	 * @param Name The name of the variable
	 */
	UFUNCTION(BlueprintCallable, Category="SUDS|Global Variables")
	void UnSetGlobalVariable(FName Name);

#if WITH_EDITORONLY_DATA
	/// Only for use by tests / editor tools when real subsystem isn't running
	static TMap<FName, FSUDSValue> Test_DummyGlobalVariables;
#endif

	
	
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
