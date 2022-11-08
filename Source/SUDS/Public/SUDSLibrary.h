#pragma once

#include "CoreMinimal.h"
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
	* @return The dialogue instance. 
	*/
	UFUNCTION(BlueprintCallable, Category="SUDS")
	static USUDSDialogue* CreateDialogue(UObject* Owner, USUDSScript* Script);

	/**
	* Create a dialogue instance based on a script, with an initial set of participants.
	* @param Owner The owner of this instance. Can be any object but determines the lifespan of this dialogue,
	*   could make sense to make the owner the NPC you're talking to for example.
	* @param Script The script to base this dialogue on
	* @param Participants List of participants, each of which must implement the ISUDSParticipant interface to be used.
	*	Participants provide parameters, variables and speaker names.
	* @return The dialogue instance. 
	*/
	UFUNCTION(BlueprintCallable, Category="SUDS")
	static USUDSDialogue* CreateDialogueWithParticipants(UObject* Owner,
	                                                     USUDSScript* Script,
	                                                     const TMap<FString, UObject*>& Participants);
	
};
