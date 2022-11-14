#include "SUDSScriptNodeEvent.h"

void USUDSScriptNodeEvent::Init(const FString& EvtName, const TArray<FSUDSValue>& Args)
{
	NodeType = ESUDSScriptNodeType::Event;
	EventName = FName(EvtName);
	LiteralArgs = Args;
	
}
