#pragma once

#include "CoreMinimal.h"
#include "SUDSTextParameters.h"
#include "UObject/Object.h"
#include "SUDSDialogue.generated.h"

struct FSUDSScriptEdge;
class USUDSScriptNode;
class USUDSScript;

DECLARE_LOG_CATEGORY_EXTERN(LogSUDSDialogue, Verbose, All);
/**
 * A Dialogue is a runtime instance of a Script (the asset on which the dialogue is based)
 * An Dialogue instance involves specific parties taking the Roles, which may be Speakers in the Dialogue
 * or may be silent, providing or receiving data from the dialogue.
 * A dialogue instance has its own set of state, so you can invoke the same Script multiple times with different
 * things filling the Roles if you want. 
 */
UCLASS(BlueprintType)
class SUDS_API USUDSDialogue : public UObject
{
	GENERATED_BODY()
protected:
	UPROPERTY()
	const USUDSScript* BaseScript;
	UPROPERTY()
	USUDSScriptNode* CurrentNode;
	/// Map of role to participant
	UPROPERTY()
	TMap<FString, UObject*> Participants;


	FSUDSTextParameters CurrentParams;
	
	/// Cached derived info
	mutable FText CurrentSpeakerDisplayName;
	/// All choices available from the current node (via a linked Choice node)
	mutable const TArray<FSUDSScriptEdge>* AllCurrentChoices;
	/// All valid choices
	mutable TArray<FSUDSScriptEdge> ValidCurrentChoices;
	static const FText DummyText;
	static const FString DummyString;

	void SetCurrentNode(USUDSScriptNode* Node);
	void RetrieveParams();
	void SortParticipants();
	const TArray<FSUDSScriptEdge>* GetChoices(bool bOnlyValidChoices) const;
public:
	USUDSDialogue();
	void Initialise(const USUDSScript* Script, FName StartLabel = NAME_None);

	/**
	 * Set the complete list of participants for this dialogue instance.
	 * Participants provide parameter values, variables, speaker names, and can receive events from the dialogue.
	 * @param NewParticipants Map of role name referred to in the dialogue script to participant object.
	 */
	UFUNCTION(BlueprintCallable)
	void SetParticipants(const TMap<FString, UObject*> NewParticipants);

	
	/**
	 * Add a participant to this dialogue instance.
	 * Participants provide parameter values, variables, speaker names, and can receive events from the dialogue.
	 * @param RoleName The role name that this participant fulfils (may be referred to in the dialogue)
	 * @param Participant The participant object
	 */
	UFUNCTION(BlueprintCallable)
	void AddParticipant(const FString& RoleName, UObject* Participant);

	/// Retrieve a participant from this dialogue
	UFUNCTION(BlueprintCallable)
	UObject* GetParticipant(const FString& RoleName);
	
	

	/// Get the speech text for the current dialogue node
	/// Any parameters required will be requested from participants in the dialogue and replaced 
	UFUNCTION(BlueprintCallable, BlueprintPure)
	FText GetText() const;

	/// Get the ID of the current speaker
	UFUNCTION(BlueprintCallable, BlueprintPure)
	const FString& GetSpeakerID() const;

	/// Get the display name of the current speaker
	UFUNCTION(BlueprintCallable, BlueprintPure)
	FText GetSpeakerDisplayName() const;

	/**
	 * Get the number of choices available from this node.
	 * Note, this will return 1 in the case of just linear text progression. The difference between just linked text
	 * lines and a choice with only 1 option is whether the choice text is blank or not.
	 * @param bOnlyValidChoices If true, only count choices which are valid given current conditions 
	 * @return The number of choices available
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	int GetNumberOfChoices(bool bOnlyValidChoices = true) const;

	/**
	 * Get the text associated with a choice.
	 * @param Index The index of the choice
	 * @param bOnlyValidChoices If true, use the valid choices list not the full list
	 * @return The text. This may be blank if this represents just a link between 2 nodes and not a choice at all.
	 *    Note that if you want to have only 1 choice but with associated text, this is fine and should be a choice
	 *    line just like any other.
	 */
	UFUNCTION(BlueprintCallable)
	FText GetChoiceText(int Index, bool bOnlyValidChoices = true) const;
	
	/**
	 * Continues the dialogue if (and ONLY if) there is only one valid path/choice out of the current node.
	 * @return True if the dialogue continues after this, false if the dialogue is now at an end.
	 */
	UFUNCTION(BlueprintCallable)
	bool Continue();
	
	/**
	 * Picks one of the available choices 
	 * If there's only 1 you can still call this with Index = 0, but also see Continue
	 * @param Index The index of the choice to make
	 * @return True if the dialogue continues, false if it has now reached the end.
	 */
	UFUNCTION(BlueprintCallable)
	bool Choose(int Index);

	/// Returns true if the dialogue has reached the end
	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool IsEnded() const;

	/**
	 * Restart the dialogue, either from the start or from a named label.
	 * @param bResetState Reset all dialogue local state, as if the dialogue had been created anew
	 * @param StartLabel Label to start running from; if None start from the beginning.
	 */
	UFUNCTION(BlueprintCallable)
	void Restart(bool bResetState = true, FName StartLabel = NAME_None);

};
