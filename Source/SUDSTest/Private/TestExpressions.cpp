#include "SUDSExpression.h"
#include "Internationalization/StringTable.h"
#include "Internationalization/StringTableRegistry.h"
#include "Misc/AutomationTest.h"

PRAGMA_DISABLE_OPTIMIZATION

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestExpressionsStandalone,
								 "SUDSTest.TestExpressionsStandalone",
								 EAutomationTestFlags::EditorContext |
								 EAutomationTestFlags::ClientContext |
								 EAutomationTestFlags::ProductFilter)



bool FTestExpressionsStandalone::RunTest(const FString& Parameters)
{
	FSUDSExpression Expr;
	TMap<FName, FSUDSValue> Variables;

	TestTrue("LiteralParse", Expr.ParseFromString("3 + 4 * 6 + 1", "LiteralParse"));

	auto& RPN = Expr.GetQueue();
	if (TestEqual("Queue len", RPN.Num(), 7))
	{
		// RPN should be 3 4 6 * + 1 +
		TestEqual("Queue 0", RPN[0].GetOperandValue().GetIntValue(), 3);
		TestEqual("Queue 1", RPN[1].GetOperandValue().GetIntValue(), 4);
		TestEqual("Queue 2", RPN[2].GetOperandValue().GetIntValue(), 6);
		TestEqual("Queue 3", RPN[3].GetType(), ESUDSExpressionItemType::Multiply);
		TestEqual("Queue 4", RPN[4].GetType(), ESUDSExpressionItemType::Add);
		TestEqual("Queue 5", RPN[5].GetOperandValue().GetIntValue(), 1);
		TestEqual("Queue 6", RPN[6].GetType(), ESUDSExpressionItemType::Add);
	}

	//TestEqual("Execute", Expr.Execute(Variables).GetIntValue(), 28);
	
	return true;
};


PRAGMA_ENABLE_OPTIMIZATION