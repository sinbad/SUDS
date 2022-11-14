#include "TestEventSub.h"

#include "SUDSDialogue.h"

void UTestEventSub::Init(USUDSDialogue* Dlg)
{
	Dlg->OnEvent.AddDynamic(this, &UTestEventSub::OnEvent);
}

void UTestEventSub::OnEvent(USUDSDialogue* Dlg, FName EventName, const TArray<FSUDSValue>& Args)
{
	EventRecords.Add(FEventRecord { EventName, Args });
}
