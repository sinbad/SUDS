#pragma once

#include "CoreMinimal.h"
#include "SUDSValue.h"
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
	* @param bStartImmediately Whether to call Start() on the dialogue automatically before returning
	* @param StartLabel If set to start immediately, which label to start from (None means start from the beginning)
	* @return The dialogue instance. 
	*/
	UFUNCTION(BlueprintCallable, Category="SUDS")
	static USUDSDialogue* CreateDialogue(UObject* Owner,
	                                     USUDSScript* Script,
	                                     bool bStartImmediately = false,
	                                     FName StartLabel = NAME_None);

	/**
	* Create a dialogue instance based on a script, with an initial set of participants.
	* @param Owner The owner of this instance. Can be any object but determines the lifespan of this dialogue,
	*   could make sense to make the owner the NPC you're talking to for example.
	* @param Script The script to base this dialogue on
	* @param Participants List of participants, each of which must implement the ISUDSParticipant interface to be used.
	*	Participants are objects that want to be closely involved in the dialogue to provide variables and receive all events.
	*	Other objects can subscribe to events separately but do not have as much control.
	* @param bStartImmediately Whether to call Start() on the dialogue automatically before returning
	* @param StartLabel If set to start immediately, which label to start from (None means start from the beginning)
	* @return The dialogue instance. 
	*/
	UFUNCTION(BlueprintCallable, Category="SUDS")
	static USUDSDialogue* CreateDialogueWithParticipants(UObject* Owner,
	                                                     USUDSScript* Script,
	                                                     const TArray<UObject*>& Participants,
	                                                     bool bStartImmediately = false,
	                                                     FName StartLabel = NAME_None);


	/**
	 * Try to extract a text value from a general SUDS value.
	 * @param Value The SUDS value, which may contain many types of value
	 * @param TextValue The text value
	 * @return True if the value was of type text and extracted correctly. False if not.
	 */
	UFUNCTION(BlueprintCallable, Category="SUDS")
	static UPARAM(DisplayName="Success") bool GetDialogueValueAsText(const FSUDSValue& Value, FText& TextValue);
	/**
	 * Try to extract a boolean value from a general SUDS value.
	 * @param Value The SUDS value, which may contain many types of value
	 * @param BoolValue The boolean value
	 * @return True if the value was of type boolean and extracted correctly. False if not.
	 */
	UFUNCTION(BlueprintCallable, Category="SUDS")
	static UPARAM(DisplayName="Success") bool GetDialogueValueAsBoolean(const FSUDSValue& Value, bool& BoolValue);
	/**
	 * Try to extract an integer value from a general SUDS value.
	 * @param Value The SUDS value, which may contain many types of value
	 * @param IntValue The integer value
	 * @return True if the value was of type integer and extracted correctly. False if not.
	 */
	UFUNCTION(BlueprintCallable, Category="SUDS")
	static UPARAM(DisplayName="Success") bool GetDialogueValueAsInt(const FSUDSValue& Value, int& IntValue);
	/**
	 * Try to extract a float value from a general SUDS value.
	 * @param Value The SUDS value, which may contain many types of value
	 * @param FloatValue The float value
	 * @return True if the value was of type float and extracted correctly. False if not.
	 */
	UFUNCTION(BlueprintCallable, Category="SUDS")
	static UPARAM(DisplayName="Success") bool GetDialogueValueAsFloat(const FSUDSValue& Value, float& FloatValue);
	/**
	 * Try to extract a gender value from a general SUDS value.
	 * @param Value The SUDS value, which may contain many types of value
	 * @param GenderValue The gender value
	 * @return True if the value was of type gender and extracted correctly. False if not.
	 */
	UFUNCTION(BlueprintCallable, Category="SUDS")
	static UPARAM(DisplayName="Success") bool GetDialogueValueAsGender(const FSUDSValue& Value, ETextGender& GenderValue);
	
};
