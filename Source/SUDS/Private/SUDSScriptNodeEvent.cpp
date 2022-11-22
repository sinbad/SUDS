#include "SUDSScriptNodeEvent.h"

void USUDSScriptNodeEvent::Init(const FString& EvtName, const TArray<FSUDSExpression>& InArgs)
{
	NodeType = ESUDSScriptNodeType::Event;
	EventName = FName(EvtName);
	Args = InArgs;
	
}
