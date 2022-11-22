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

	Variables.Add("Six", 6);
	TestTrue("SimpleVarParse", Expr.ParseFromString("3 + 4 * {Six} + 1", "SimpleVarParse"));
	TestEqual("Eval", Expr.Evaluate(Variables).GetIntValue(), 28);
	
	auto& RPN = Expr.GetQueue();
	if (TestEqual("Queue len", RPN.Num(), 7))
	{
		// RPN should be 3 4 6 * + 1 +
		TestEqual("Queue 0", RPN[0].GetOperandValue().GetIntValue(), 3);
		TestEqual("Queue 1", RPN[1].GetOperandValue().GetIntValue(), 4);
		TestEqual("Queue 2", RPN[2].GetOperandValue().GetVariableNameValue().ToString(), "Six");
		TestEqual("Queue 3", RPN[3].GetType(), ESUDSExpressionItemType::Multiply);
		TestEqual("Queue 4", RPN[4].GetType(), ESUDSExpressionItemType::Add);
		TestEqual("Queue 5", RPN[5].GetOperandValue().GetIntValue(), 1);
		TestEqual("Queue 6", RPN[6].GetType(), ESUDSExpressionItemType::Add);
	}

	// Explicit FSUDSValue(true) needed to avoid it using the int conversion by default
	Variables.Add("IsATest", FSUDSValue(true));
	TestTrue("BoolSingleValueParse", Expr.ParseFromString("{IsATest}", "BoolSingleValueParse"));
	TestTrue("Eval", Expr.Evaluate(Variables).GetBooleanValue());
	
	Variables.Add("SomethingFalse", FSUDSValue(false));
	Variables.Add("SomethingTrue", FSUDSValue(true));
	Variables.Add("SomethingElseFalse", FSUDSValue(false));
	TestTrue("BoolCompound1", Expr.ParseFromString("!{SomethingFalse} && {SomethingTrue}", "BoolCompound1"));
	TestTrue("Eval", Expr.Evaluate(Variables).GetBooleanValue());
	TestTrue("BoolCompound2", Expr.ParseFromString("{SomethingFalse} || {SomethingTrue}", "BoolCompound2"));
	TestTrue("Eval", Expr.Evaluate(Variables).GetBooleanValue());
	TestTrue("BoolCompound3", Expr.ParseFromString("{SomethingFalse} or {SomethingTrue}", "BoolCompound3"));
	TestTrue("Eval", Expr.Evaluate(Variables).GetBooleanValue());
	// Test parentheses changing precedence & result
	// True result for successful parsing, but false for Eval unless we parenthesise
	TestTrue("BoolCompound4", Expr.ParseFromString("!{SomethingFalse} && {SomethingElseFalse} && {SomethingTrue}", "BoolCompound4"));
	TestFalse("Eval", Expr.Evaluate(Variables).GetBooleanValue());
	TestTrue("BoolCompound5", Expr.ParseFromString("!({SomethingFalse} && {SomethingElseFalse}) && {SomethingTrue}", "BoolCompound5"));
	TestTrue("Eval", Expr.Evaluate(Variables).GetBooleanValue());
	TestTrue("BoolCompound6", Expr.ParseFromString("not {SomethingFalse} and {SomethingElseFalse} and {SomethingTrue}", "BoolCompound6"));
	TestFalse("Eval", Expr.Evaluate(Variables).GetBooleanValue());
	TestTrue("BoolCompound7", Expr.ParseFromString("not ({SomethingFalse} and {SomethingElseFalse}) and {SomethingTrue}", "BoolCompound7"));
	TestTrue("Eval", Expr.Evaluate(Variables).GetBooleanValue());

	return true;
};


PRAGMA_ENABLE_OPTIMIZATION