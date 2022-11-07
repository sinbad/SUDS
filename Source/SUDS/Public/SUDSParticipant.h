#pragma once

#include "CoreMinimal.h"
#include "SUDSTextParameters.h"
#include "UObject/Interface.h"
#include "SUDSParticipant.generated.h"

class USUDSDialogue;
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
	 * Trigger for this participant to update any parameters to be replaced in the text.
	 * This function will be called each time the dialogue progresses.
	 * @param Dialogue The dialogue requesting parameters
	 * @param Params The parameters to be updated. Call SetParameter from C++, or from Blueprints
	 *		one of the BPL functions SetDialogueParameter[Type] 
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="SUDS")
	void UpdateDialogueParameters(USUDSDialogue* Dialogue, UPARAM(ref) FSUDSTextParameters& Params);	

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
	 * When text parameters are set by participants, they do it in order every time the dialogue progresses.
	 * If for some reason you need to control the order because it's possible more than one participant could try to
	 * set the same variable, override this method; higher priority values override lower ones (and negatives are allowed).
	 * @return 
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="SUDS")
	int GetDialogueParticipantPriority() const;

};


