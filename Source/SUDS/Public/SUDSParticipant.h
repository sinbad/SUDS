#pragma once

#include "CoreMinimal.h"
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
* A participant could be a speaker, or simply a provider of variables for the dialogue.
* While any event listener on the dialogue can provide similar functionality, implementing this interface is a bit
* simpler than subscribing, and participants are *guaranteed* to be called earlier than event listeners.
*
* Probably you want to implement providers of variables etc as ISUDSParticipant, and UIs on the dialogue as USUDSDialogue event
* listeners.
* implemented
* 
*/
class SUDS_API ISUDSParticipant
{
	GENERATED_BODY()

public:

	/**
	 * Called when a dialogue involving this participant is starting.
	 * The implementation should probably set any starting variables referenced by the dialogue here (or you can do that
	 * later during other functions). At this point there is no active speaker line, we're bootstrapping.
	 * @param Dialogue The dialogue
	 * @param AtLabel The label that the dialogue has started at (None if starting at the beginning)
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="SUDS")
	void OnDialogueStarting(USUDSDialogue* Dialogue, FName AtLabel);	

	/**
	 * Called when a dialogue finishes.
	 * @param Dialogue The dialogue
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="SUDS")
	void OnDialogueFinished(USUDSDialogue* Dialogue);	

	/**
	 * Called when a new speaker line, potentially with attached choices, has become active in the dialogue.
	 * This participant can provide any variable updates if it needs to at this point.
	 * Participants will be called before any dialogue event listeners.
	 * @param Dialogue The dialogue
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="SUDS")
	void OnDialogueSpeakerLine(USUDSDialogue* Dialogue);	

	/**
	 * Called when a choice is made by the player.
	 * At this point, the dialogue has not progressed as a result of that choice, so the index passed can be used to
	 * reference the choice.
	 * Participants will be called before any dialogue event listeners.
	 * @param Dialogue The dialogue
	 * @param ChoiceIndex The index of the choice that was made
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="SUDS")
	void OnDialogueChoiceMade(USUDSDialogue* Dialogue, int ChoiceIndex);	
	
	/**
	 * Return the priority of this participant (default 0).
	 * If for some reason you need to control the order multiple participants in a dialogue are called, 
	 * override this method; higher priority participants will be called *later* so that their variables etc override
	 * previously set values.
	 * @return Relative priority, default 0, higher numbers override lower ones. 
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="SUDS")
	int GetDialogueParticipantPriority() const;

};


