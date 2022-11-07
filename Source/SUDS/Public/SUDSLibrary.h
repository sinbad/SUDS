#pragma once

#include "CoreMinimal.h"
#include "SUDSTextParameters.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SUDSLibrary.generated.h"

class USUDSScript;
class USUDSDialogue;
UCLASS()
class SUDS_API USUDSLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	/**
	* Create a dialogue instance based on a script, with no participants.
	* You should subsequently call "SetParticipants" or "AddParticipant" on the returned dialogue if you expect any
	* parameters or speaker names to work.
	* @param Owner The owner of this instance. Can be any object but determines the lifespan of this dialogue,
	*   could make sense to make the owner the NPC you're talking to for example.
	* @param Script The script to base this dialogue on
	* @param StartAtLabel The label to start at. If none, start at the beginning.
	* @return The dialogue instance. 
	*/
	UFUNCTION(BlueprintCallable, Category="SUDS")
	static USUDSDialogue* CreateDialogue(UObject* Owner, USUDSScript* Script, FName StartAtLabel = NAME_None);

	/**
	* Create a dialogue instance based on a script, with an initial set of participants.
	* @param Owner The owner of this instance. Can be any object but determines the lifespan of this dialogue,
	*   could make sense to make the owner the NPC you're talking to for example.
	* @param Script The script to base this dialogue on
	* @param Participants List of participants, each of which must implement the ISUDSParticipant interface to be used.
	*	Participants provide parameters, variables and speaker names.
	* @param StartAtLabel The label to start at. If none, start at the beginning.
	* @return The dialogue instance. 
	*/
	UFUNCTION(BlueprintCallable, Category="SUDS")
	static USUDSDialogue* CreateDialogueWithParticipants(UObject* Owner,
	                                                     USUDSScript* Script,
	                                                     const TMap<FString, UObject*>& Participants,
	                                                     FName StartAtLabel = NAME_None);

	// Blueprint wrapper functions for setting text parameters
	
	/**
	 * Set a dialogue parameter on the passed in parameters collection.
	 * @param Params The parameter collection to update
	 * @param Name The name of the parameter
	 * @param Value The value of the parameter
	 */
	UFUNCTION(BlueprintCallable, Category="SUDS")
	static void SetDialogueTextParameter(UPARAM(ref) FSUDSTextParameters& Params, FString Name, FText Value)
	{
		Params.SetParameter(Name, Value);
	}

	/**
	 * Set a dialogue parameter on the passed in parameters collection.
	 * @param Params The parameter collection to update
	 * @param Name The name of the parameter
	 * @param Value The value of the parameter
	 */
	UFUNCTION(BlueprintCallable, Category="SUDS")
	static void SetDialogueIntParameter(UPARAM(ref) FSUDSTextParameters& Params, FString Name, int32 Value)
	{
		Params.SetParameter(Name, Value);
	}

	/**
	 * Set a dialogue parameter on the passed in parameters collection.
	 * @param Params The parameter collection to update
	 * @param Name The name of the parameter
	 * @param Value The value of the parameter
	 */
	UFUNCTION(BlueprintCallable, Category="SUDS")
	static void SetDialogueInt64Parameter(UPARAM(ref) FSUDSTextParameters& Params, FString Name, int64 Value)
	{
		Params.SetParameter(Name, Value);
	}

	/**
	 * Set a dialogue parameter on the passed in parameters collection.
	 * @param Params The parameter collection to update
	 * @param Name The name of the parameter
	 * @param Value The value of the parameter
	 */
	UFUNCTION(BlueprintCallable, Category="SUDS")
	static void SetDialogueFloatParameter(UPARAM(ref) FSUDSTextParameters& Params, FString Name, float Value)
	{
		Params.SetParameter(Name, Value);
	}

	/**
	 * Set a dialogue parameter on the passed in parameters collection.
	 * @param Params The parameter collection to update
	 * @param Name The name of the parameter
	 * @param Value The value of the parameter
	 */
	UFUNCTION(BlueprintCallable, Category="SUDS")
	static void SetDialogueGenderParameter(UPARAM(ref) FSUDSTextParameters& Params, FString Name, ETextGender Value)
	{
		Params.SetParameter(Name, Value);
	}
	
	
};
