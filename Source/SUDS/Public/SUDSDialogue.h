#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SUDSDialogue.generated.h"

struct FSUDSScriptEdge;
class USUDSScriptNode;
class USUDSScript;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDialogueSpeakerLine, class USUDSDialogue*, Dialogue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDialogueChoice, class USUDSDialogue*, Dialogue, int, ChoiceIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDialogueStarting, class USUDSDialogue*, Dialogue, FName, AtLabel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDialogueFinished, class USUDSDialogue*, Dialogue);

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
public:
	/// Event raised when dialogue progresses and a new speaker line, potentially with new choices, is ready to be displayed
	FOnDialogueSpeakerLine OnSpeakerLine;
	/// Event raised when a choice is made in the dialogue by the player. At this point, the dialogue has not progressed
	/// as a result of that choice so the index passed can be used to reference the choice
	FOnDialogueChoice OnChoice;
	/// Event raised when the dialogue is starting, before the first speaker line
	FOnDialogueStarting OnStarting;
	/// Event raised when the dialogue finishes
	FOnDialogueFinished OnFinished;
protected:
	UPROPERTY()
	const USUDSScript* BaseScript;
	UPROPERTY()
	USUDSScriptNode* CurrentNode;
	/// Map of role to participant
	UPROPERTY()
	TMap<FString, UObject*> Participants;

	/// All of the dialogue variables
	/// Dialogue variable state is all held locally. Dialogue participants can retrieve or set values in state.
	/// All state is saved with the dialogue. Variables can be used as text substitution parameters, conditionals,
	/// or communication with external state.
	FFormatNamedArguments VariableState;


	TSet<FString> CurrentRequestedParamNames;
	bool bParamNamesExtracted;
	
	/// Cached derived info
	mutable FText CurrentSpeakerDisplayName;
	/// All choices available from the current node (via a linked Choice node)
	mutable const TArray<FSUDSScriptEdge>* AllCurrentChoices;
	/// All valid choices
	mutable TArray<FSUDSScriptEdge> ValidCurrentChoices;
	static const FText DummyText;
	static const FString DummyString;

	void SetCurrentNode(USUDSScriptNode* Node);
	void SortParticipants();
	void RaiseStarting(FName StartLabel);
	void RaiseFinished();
	void RaiseNewSpeakerLine();
	void RaiseChoiceMade(int Index);

	const TArray<FSUDSScriptEdge>* GetChoices(bool bOnlyValidChoices) const;
public:
	USUDSDialogue();
	void Initialise(const USUDSScript* Script);

	/**
	 * Set the complete list of participants for this dialogue instance.
	 * Participants provide parameter values, variables, speaker names, and can receive events from the dialogue.
	 * @param NewParticipants Map of role name referred to in the dialogue script to participant object.
	 */
	UFUNCTION(BlueprintCallable)
	void SetParticipants(const TMap<FString, UObject*> NewParticipants);

	/**
	 * Begin the dialogue. Make sure you've added all participants before calling this.
	 * This may not be the first time you've started this dialogue. All previous state is maintained to enable you
	 * for example to take branching paths based on whether you've spoken to this character before.
	 * If you want to reset *all* state, call Restart(true). However this is an extreme case; if you want to just
	 * reset some variables then use the header section of the script to set variables to a default starting point.
	 * @param Label The start point for this dialogue. If None, starts from the beginning.
	 */
	UFUNCTION(BlueprintCallable)
	void Start(FName Label = NAME_None);


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
	 * @param bResetState Reset all dialogue local state, as if the dialogue had been created anew. You mostly don't want
	 * to do this; if you have certain things you want to reset every time, then use [set] commands in the header section
	 * which runs every time the dialogue starts.
	 * @param StartLabel Label to start running from; if None start from the beginning.
	 */
	UFUNCTION(BlueprintCallable)
	void Restart(bool bResetState = false, FName StartLabel = NAME_None);

	/// Get the set of text parameters that are actually being asked for in the current state of the dialogue.
	/// This will include parameters in the text, and parameters in any current choices being displayed.
	/// Use this if you want to be more specific about what parameters you supply when ISUDSParticipant::UpdateDialogueParameters
	/// is called.
	UFUNCTION(BlueprintCallable)
	TSet<FString> GetParametersInUse();


	/// Set a variable in dialogue state
	template <typename T>
	void SetVariable(const FString& Name, const T& Value)
	{
		VariableState.Add(Name, Value);
	}

	/**
	 * Set a text dialogue variable
	 * @param Name The name of the variable
	 * @param Value The value of the variable
	 */
	UFUNCTION(BlueprintCallable)
	void SetVariableText(FString Name, FText Value)
	{
		VariableState.Add(Name, Value);
	}

	/**
	 * Get a text dialogue variable
	 * @param Name The name of the variable
	 * @returns Value The value of the variable
	 */
	UFUNCTION(BlueprintCallable)
	FText GetVariableText(FString Name)
	{
		if (auto Arg = VariableState.Find(Name))
		{
			if (Arg->GetType() == EFormatArgumentType::Text)
			{
				return Arg->GetTextValue();
			}
			else
			{
				UE_LOG(LogSUDSDialogue, Error, TEXT("Requested variable %s of type text but was not a compatible type"), *Name);
			}
		}
		return FText();
	}

	/**
	 * Set a dialogue variable on the passed in parameters collection.
	 * @param Name The name of the variable
	 * @param Value The value of the variable
	 */
	UFUNCTION(BlueprintCallable)
	void SetVariableInt(FString Name, int32 Value)
	{
		VariableState.Add(Name, Value);
	}

	/**
	 * Get an int dialogue variable
	 * @param Name The name of the variable
	 * @returns Value The value of the variable
	 */
	UFUNCTION(BlueprintCallable)
	int GetVariableInt(FString Name)
	{
		if (auto Arg = VariableState.Find(Name))
		{
			switch (Arg->GetType())
			{
			case EFormatArgumentType::Int:
				return Arg->GetIntValue();
			case EFormatArgumentType::UInt:
				UE_LOG(LogSUDSDialogue, Warning, TEXT("Casting variable %s to int, data loss may occur"), *Name);
				return Arg->GetUIntValue();
			case EFormatArgumentType::Float:
				UE_LOG(LogSUDSDialogue, Warning, TEXT("Casting variable %s to int, data loss may occur"), *Name);
				return Arg->GetFloatValue();
			case EFormatArgumentType::Double:
				UE_LOG(LogSUDSDialogue, Warning, TEXT("Casting variable %s to int, data loss may occur"), *Name);
				return Arg->GetDoubleValue();
			default: 
			case EFormatArgumentType::Gender:
			case EFormatArgumentType::Text:
				UE_LOG(LogSUDSDialogue, Error, TEXT("Variable %s is not a compatible integer type"), *Name);
			}
		}
		return 0;
	}
	
	/**
	 * Set an Int64 dialogue variable
	 * @param Name The name of the parameter
	 * @param Value The value of the parameter
	 */
	UFUNCTION(BlueprintCallable)
	void SetVariableInt64(FString Name, int64 Value)
	{
		VariableState.Add(Name, Value);
	}

	/**
	 * Get an Int64 dialogue variable
	 * @param Name The name of the variable
	 * @returns Value The value of the variable
	 */
	UFUNCTION(BlueprintCallable)
	int64 GetVariableInt64(FString Name)
	{
		if (auto Arg = VariableState.Find(Name))
		{
			switch (Arg->GetType())
			{
			case EFormatArgumentType::Int:
				return Arg->GetIntValue();
			case EFormatArgumentType::UInt:
				UE_LOG(LogSUDSDialogue, Warning, TEXT("Casting variable %s to int, data loss may occur"), *Name);
				return Arg->GetUIntValue();
			case EFormatArgumentType::Float:
				UE_LOG(LogSUDSDialogue, Warning, TEXT("Casting variable %s to int, data loss may occur"), *Name);
				return Arg->GetFloatValue();
			case EFormatArgumentType::Double:
				UE_LOG(LogSUDSDialogue, Warning, TEXT("Casting variable %s to int, data loss may occur"), *Name);
				return Arg->GetDoubleValue();
			default: 
			case EFormatArgumentType::Gender:
			case EFormatArgumentType::Text:
				UE_LOG(LogSUDSDialogue, Error, TEXT("Variable %s is not a compatible integer type"), *Name);
			}
		}
		return 0;
	}

	/**
	 * Set a float dialogue variable 
	 * @param Name The name of the variable
	 * @param Value The value of the variable
	 */
	UFUNCTION(BlueprintCallable)
	void SetVariableFloat(FString Name, float Value)
	{
		VariableState.Add(Name, Value);
	}

	/**
	 * Get a float dialogue variable
	 * @param Name The name of the variable
	 * @returns Value The value of the variable
	 */
	UFUNCTION(BlueprintCallable)
	float GetVariableFloat(FString Name)
	{
		if (auto Arg = VariableState.Find(Name))
		{
			switch (Arg->GetType())
			{
			case EFormatArgumentType::Int:
				return Arg->GetIntValue();
			case EFormatArgumentType::UInt:
				return Arg->GetUIntValue();
			case EFormatArgumentType::Float:
				return Arg->GetFloatValue();
			case EFormatArgumentType::Double:
				UE_LOG(LogSUDSDialogue, Warning, TEXT("Casting variable %s to float, data loss may occur"), *Name);
				return Arg->GetDoubleValue();
			default: 
			case EFormatArgumentType::Gender:
			case EFormatArgumentType::Text:
				UE_LOG(LogSUDSDialogue, Error, TEXT("Variable %s is not a compatible float type"), *Name);
			}
		}
		return 0;
	}
	
	/**
	 * Set a gender dialogue variable 
	 * @param Name The name of the variable
	 * @param Value The value of the variable
	 */
	UFUNCTION(BlueprintCallable)
	void SetVariableGender(FString Name, ETextGender Value)
	{
		VariableState.Add(Name, Value);
	}

	/**
	 * Get a gender dialogue variable
	 * @param Name The name of the variable
	 * @returns Value The value of the variable
	 */
	UFUNCTION(BlueprintCallable)
	ETextGender GetVariableGender(FString Name)
	{
		if (auto Arg = VariableState.Find(Name))
		{
			switch (Arg->GetType())
			{
			case EFormatArgumentType::Gender:
				return Arg->GetGenderValue();
			default: 
			case EFormatArgumentType::Int:
			case EFormatArgumentType::UInt:
			case EFormatArgumentType::Float:
			case EFormatArgumentType::Double:
			case EFormatArgumentType::Text:
				UE_LOG(LogSUDSDialogue, Error, TEXT("Variable %s is not a compatible gender type"), *Name);
			}
		}
		return ETextGender::Neuter;
	}

	/**
	 * Set a boolean dialogue variable 
	 * @param Name The name of the variable
	 * @param Value The value of the variable
	 */
	UFUNCTION(BlueprintCallable)
	void SetVariableBoolean(FString Name, bool Value)
	{
		VariableState.Add(Name, Value);
	}

	/**
	 * Get a boolean dialogue variable
	 * @param Name The name of the variable
	 * @returns Value The value of the variable
	 */
	UFUNCTION(BlueprintCallable)
	bool GetVariableBoolean(FString Name)
	{
		if (auto Arg = VariableState.Find(Name))
		{
			switch (Arg->GetType())
			{
			case EFormatArgumentType::Int:
			case EFormatArgumentType::UInt:
				return Arg->GetIntValue() != 0;
			default: 
			case EFormatArgumentType::Gender:
			case EFormatArgumentType::Float:
			case EFormatArgumentType::Double:
			case EFormatArgumentType::Text:
				UE_LOG(LogSUDSDialogue, Error, TEXT("Variable %s is not a compatible boolean type"), *Name);
			}
		}
		return false;
	}

};
