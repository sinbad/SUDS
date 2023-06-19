// Copyright Steve Streeting 2022
// Released under the MIT license https://opensource.org/license/MIT/
#include "SUDSScriptNodeEvent.h"

void USUDSScriptNodeEvent::Init(const FString& EvtName, const TArray<FSUDSExpression>& InArgs, int LineNo)
{
	NodeType = ESUDSScriptNodeType::Event;
	EventName = FName(EvtName);
	Args = InArgs;
	SourceLineNo = LineNo;
	
}
