// Copyright Steve Streeting 2022
// Released under the MIT license https://opensource.org/license/MIT/
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SUDSValue.h"
#include "SUDSParticipant.generated.h"

class USUDSDialogue;
UINTERFACE(MinimalAPI)
class USUDSParticipant : public UInterface
{
	GENERATED_BODY()
};

/**
* Interface to be implemented by participant objects in a given dialogue.
* A participant is simply any object which wants to be closely involved in supplying data to, or retrieving data from,
* the dialogue. Although you could do this simply by subscribing to the delegate events on a dialogue, the advantage
* of making a participant is that you have better control over the ordering of multiple participants, in case for example
* there's some common variable that they both want to set.
* It's also a clearer interface to look for vs ad-hoc delegate hooks.
* Generally we recommend that:
*   - Objects providing data to the dialogue should implement ISUDSParticipant
*   - Anything that just wants to observe the dialogue (like a UI) should just listen to events
*   
* Participants are *guaranteed* to be called earlier than delegates, which means you can set variables from the
* Participant callback and when the UI delegate reads text back, all substitution variables will be up to date.
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
	 * This event is ONLY raised if there's a choice of paths, not for just continuing a linear path.
	 * See OnDialogueProceeding for a more general callback.
	 * Participants will be called before any dialogue event listeners.
	 * @param Dialogue The dialogue
	 * @param ChoiceIndex The index of the choice that was made
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="SUDS")
	void OnDialogueChoiceMade(USUDSDialogue* Dialogue, int ChoiceIndex);

	/**
	 * Called just before proceeding with the dialogue from the current speaker line; just after either a choice is made by the player
	 * or the dialogue is just prompted to proceed with its single path.
	 * Participants will be called before any dialogue event listeners.
	 * @param Dialogue The dialogue
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="SUDS")
	void OnDialogueProceeding(USUDSDialogue* Dialogue);
	

	/**
	 * Called when an event is raised from dialogue 
	 * @param Dialogue The dialogue instance
	 * @param EventName The name of the event that has been raised
	 * @param Arguments 
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="SUDS")
	void OnDialogueEvent(USUDSDialogue* Dialogue, FName EventName, const TArray<FSUDSValue>& Arguments);
	
	/**
	 * Called when a variable changes value in the dialogue 
	 * @param Dialogue The dialogue instance
	 * @param VariableName The name of the variable which has changed value
	 * @param Value The new value
	 * @param bFromScript True if the value changed because of a script line, false if it changed because of code calling SetVariable
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="SUDS")
	void OnDialogueVariableChanged(USUDSDialogue* Dialogue, FName VariableName, const FSUDSValue& Value, bool bFromScript);
	
	/**
	 * Called when a variable value is requested by the dialogue script.
	 * While you can set variables on the dialogue at any time and they're persistent, you can implement this method to
	 * provide on-demand variable values (call SetVariable on the dialogue) if you want. This hook is called just before
	 * the variables are used.
	 * @param Dialogue The dialogue instance
	 * @param VariableName The name of the variable which has changed value
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="SUDS")
	void OnDialogueVariableRequested(USUDSDialogue* Dialogue, FName VariableName);
	
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


