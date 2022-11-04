#pragma once

#include "CoreMinimal.h"
#include "SUDSTextParameters.h"
#include "UObject/Interface.h"
#include "SUDSParticipant.generated.h"

UINTERFACE(MinimalAPI)
class USUDSParticipant : public UInterface
{
	GENERATED_BODY()
};

/**
* Interface to be implemented by any participant in a given dialogue.
* A participant could be a speaker, or simply a provider of variables or text parameters (or all of the above)
* When the dialogue needs something external to itself, or raises an event, it will call participants.
* Every participant is identified by a FName for that specific dialogue; although you can use the same FName across
* all dialogues for consistency, you don't have to. So long as the dialogue can resolve a name to a participant, it
* doesn't matter what it is.
* A dialogue may either send messages to a specific participant, or to all participants, depending on how it's written.
*/
class SUDS_API ISUDSParticipant
{
	GENERATED_BODY()

public:
	
	/**
	 * Retrieve a text parameter value for completion in a dialogue line, e.g. "Hello {PlayerName}" will create
	 * a request for a "PlayerName" parameter. Implementations can either fill in the value and return true, or pass
	 * on this opportunity and return false. All participants will be asked for the parameter value.
	 * @param ParamName The name of the parameter being requested
	 * @param Params The parameter collection; if this instance can provide the requested parameter, it should call
	 *	the appropriate SetParameter method on this object and return true.
	 * @return True if the parameter was successfully set, false if this instance did not provide it.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="SUDS")
	UPARAM(DisplayName="Success") bool GetDialogueParameter(const FString& ParamName, USUDSTextParameters* Params);	


	/**
	 * Retrieve the speaker display name for this participant.
	 * This function will be called if the name that this participant was registered as is used as a speaker in a text line.
	 * It will be called for every line, just in case you want to change how the speaker is reported over the life of the
	 * dialogue.
	 * @return The speaker display name (localised)
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="SUDS")
	FText GetDialogueSpeakerDisplayName();

	/**
	 * Return the priority of this participant (default 0).
	 * When a text parameter is requested for the dialogue, participants are asked to provide it. The first one to
	 * return true from GetDialogueParameter will get to set the parameter. If for some reason you need to control the order
	 * of the requests, override this method; higher priorities are called first (and negatives are allowed).
	 * @return 
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="SUDS")
	int GetDialogueParticipantPriority() const;

};


