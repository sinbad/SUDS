

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
	* Create a dialogue instance based on a script.
	* @param Owner The owner of this instance. Can be any object but determines the lifespan of this dialogue,
	*   could make sense to make the owner the NPC you're talking to for example.
	* @param Script The script to base this dialogue on
	* @param StartAtLabel The label to start at. If none, start at the beginning.
	* @return The dialogue instance. 
	*/
	UFUNCTION(BlueprintCallable, Category="SUDS")
	static USUDSDialogue* CreateDialogue(UObject* Owner, USUDSScript* Script, FName StartAtLabel = NAME_None);

};
