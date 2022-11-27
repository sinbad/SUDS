#include "TestEventSub.h"

#include "SUDSDialogue.h"

void UTestEventSub::Init(USUDSDialogue* Dlg)
{
	Dlg->OnEvent.AddDynamic(this, &UTestEventSub::OnEvent);
	Dlg->OnVariableChanged.AddDynamic(this, &UTestEventSub::OnVariableChanged);

}

void UTestEventSub::OnEvent(USUDSDialogue* Dlg, FName EventName, const TArray<FSUDSValue>& Args)
{
	EventRecords.Add(FEventRecord { EventName, Args });
}

void UTestEventSub::OnVariableChanged(USUDSDialogue* Dlg, FName VarName, const FSUDSValue& Value, bool bFromScript)
{
	SetVarRecords.Add(FSetVarRecord { VarName, Value, bFromScript });
}
