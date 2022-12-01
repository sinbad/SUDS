#include "SUDSExpression.h"
#include "Internationalization/StringTable.h"
#include "Internationalization/StringTableRegistry.h"
#include "Misc/AutomationTest.h"

PRAGMA_DISABLE_OPTIMIZATION

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestExpressions,
								 "SUDSTest.TestExpressions",
								 EAutomationTestFlags::EditorContext |
								 EAutomationTestFlags::ClientContext |
								 EAutomationTestFlags::ProductFilter)



bool FTestExpressions::RunTest(const FString& Parameters)
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
	if (TestEqual("Variable count", Expr.GetVariableNames().Num(), 1))
	{
		TestEqual("Variable name", Expr.GetVariableNames()[0].ToString(), "Six");
	}

	TestTrue("Arithmetic", Expr.ParseFromString("-6.7 * 2 + (21.3 - 8) * 5", "Arithmetic"));
	TestEqual("Eval", Expr.Evaluate(Variables).GetFloatValue(), 53.1f);
	
	// Explicit FSUDSValue(true) needed to avoid it using the int conversion by default
	Variables.Add("IsATest", FSUDSValue(true));
	TestTrue("BoolSingleValueParse", Expr.ParseFromString("{IsATest}", "BoolSingleValueParse"));
	TestTrue("Eval", Expr.Evaluate(Variables).GetBooleanValue());
	if (TestEqual("Variable count", Expr.GetVariableNames().Num(), 1))
	{
		TestEqual("Variable name", Expr.GetVariableNames()[0].ToString(), "IsATest");
	}
	
	Variables.Add("SomethingFalse", FSUDSValue(false));
	Variables.Add("SomethingTrue", FSUDSValue(true));
	Variables.Add("SomethingElseFalse", FSUDSValue(false));
	TestTrue("BoolCompound1", Expr.ParseFromString("!{SomethingFalse} && {SomethingTrue}", "BoolCompound1"));
	TestTrue("Eval", Expr.Evaluate(Variables).GetBooleanValue());
	if (TestEqual("Variable count", Expr.GetVariableNames().Num(), 2))
	{
		TestEqual("Variable name", Expr.GetVariableNames()[0].ToString(), "SomethingFalse");
		TestEqual("Variable name", Expr.GetVariableNames()[1].ToString(), "SomethingTrue");
	}
	TestTrue("BoolCompound2", Expr.ParseFromString("{SomethingFalse} || {SomethingTrue}", "BoolCompound2"));
	TestTrue("Eval", Expr.Evaluate(Variables).GetBooleanValue());
	TestTrue("BoolCompound3", Expr.ParseFromString("{SomethingFalse} or {SomethingTrue}", "BoolCompound3"));
	TestTrue("Eval", Expr.Evaluate(Variables).GetBooleanValue());
	// Test parentheses changing precedence & result
	// True result for successful parsing, but false for Eval unless we parenthesise
	TestTrue("BoolCompound4", Expr.ParseFromString("!{SomethingFalse} && {SomethingElseFalse} && {SomethingTrue}", "BoolCompound4"));
	TestFalse("Eval", Expr.Evaluate(Variables).GetBooleanValue());
	if (TestEqual("Variable count", Expr.GetVariableNames().Num(), 3))
	{
		TestEqual("Variable name", Expr.GetVariableNames()[0].ToString(), "SomethingFalse");
		TestEqual("Variable name", Expr.GetVariableNames()[1].ToString(), "SomethingElseFalse");
		TestEqual("Variable name", Expr.GetVariableNames()[2].ToString(), "SomethingTrue");
	}
	TestTrue("BoolCompound5", Expr.ParseFromString("!({SomethingFalse} && {SomethingElseFalse}) && {SomethingTrue}", "BoolCompound5"));
	TestTrue("Eval", Expr.Evaluate(Variables).GetBooleanValue());
	TestTrue("BoolCompound6", Expr.ParseFromString("not {SomethingFalse} and {SomethingElseFalse} and {SomethingTrue}", "BoolCompound6"));
	TestFalse("Eval", Expr.Evaluate(Variables).GetBooleanValue());
	TestTrue("BoolCompound7", Expr.ParseFromString("not ({SomethingFalse} and {SomethingElseFalse}) and {SomethingTrue}", "BoolCompound7"));
	TestTrue("Eval", Expr.Evaluate(Variables).GetBooleanValue());

	Variables.Add("Seven", 7);
	TestTrue("Comparisons", Expr.ParseFromString("{Six} == 6", ""));
	TestTrue("Eval", Expr.Evaluate(Variables).GetBooleanValue());
	TestTrue("Comparisons", Expr.ParseFromString("{Six} = 6", ""));
	TestTrue("Eval", Expr.Evaluate(Variables).GetBooleanValue());
	TestTrue("Comparisons", Expr.ParseFromString("{Six} >= 6", ""));
	TestTrue("Eval", Expr.Evaluate(Variables).GetBooleanValue());
	TestTrue("Comparisons", Expr.ParseFromString("{Six} > 6", ""));
	TestFalse("Eval", Expr.Evaluate(Variables).GetBooleanValue());
	TestTrue("Comparisons", Expr.ParseFromString("{Six} < 6", ""));
	TestFalse("Eval", Expr.Evaluate(Variables).GetBooleanValue());
	TestTrue("Comparisons", Expr.ParseFromString("{Six} <= 6", ""));
	TestTrue("Eval", Expr.Evaluate(Variables).GetBooleanValue());
	TestTrue("Comparisons", Expr.ParseFromString("{Six} < {Seven}", ""));
	TestTrue("Eval", Expr.Evaluate(Variables).GetBooleanValue());
	TestTrue("Comparisons", Expr.ParseFromString("{Seven} > {Six}", ""));
	TestTrue("Eval", Expr.Evaluate(Variables).GetBooleanValue());
	TestTrue("Comparisons", Expr.ParseFromString("{Seven} != {Six}", ""));
	TestTrue("Eval", Expr.Evaluate(Variables).GetBooleanValue());
	TestTrue("Comparisons", Expr.ParseFromString("{Seven} != {Seven}", ""));
	TestFalse("Eval", Expr.Evaluate(Variables).GetBooleanValue());

	// Mixed float/int comparisons
	Variables.Add("EightFloat", 8.1f);
	TestTrue("Comparisons", Expr.ParseFromString("{EightFloat} > 8", ""));
	TestTrue("Eval", Expr.Evaluate(Variables).GetBooleanValue());
	TestTrue("Comparisons", Expr.ParseFromString("{EightFloat} > {Seven}", ""));
	TestTrue("Eval", Expr.Evaluate(Variables).GetBooleanValue());
	TestTrue("Comparisons", Expr.ParseFromString("{Six} < {EightFloat}", ""));
	TestTrue("Eval", Expr.Evaluate(Variables).GetBooleanValue());
	// Fuzzy float comparisons
	Variables.Add("EightFloatPlusMargin", 8.1000002f);
	TestTrue("Comparisons", Expr.ParseFromString("{EightFloat} == 8.1", ""));
	TestTrue("Eval", Expr.Evaluate(Variables).GetBooleanValue());
	TestTrue("Comparisons", Expr.ParseFromString("{EightFloat} == 8.15", ""));
	TestFalse("Eval", Expr.Evaluate(Variables).GetBooleanValue());
	TestTrue("Comparisons", Expr.ParseFromString("{EightFloatPlusMargin} == 8.1", ""));
	TestTrue("Eval", Expr.Evaluate(Variables).GetBooleanValue());
	TestTrue("Comparisons", Expr.ParseFromString("{EightFloat} == 8.1000005", ""));
	TestTrue("Eval", Expr.Evaluate(Variables).GetBooleanValue());
	TestTrue("Comparisons", Expr.ParseFromString("{EightFloatPlusMargin} == 8.1000005", ""));
	TestTrue("Eval", Expr.Evaluate(Variables).GetBooleanValue());

	// Other type comparisons
	Variables.Add("SomeText", FText::FromString("Hello"));
	TestTrue("Comparisons", Expr.ParseFromString("{SomeText} == \"Hello\"", ""));
	TestTrue("Eval", Expr.Evaluate(Variables).GetBooleanValue());
	TestTrue("Comparisons", Expr.ParseFromString("{SomeText} == \"Hi\"", ""));
	TestFalse("Eval", Expr.Evaluate(Variables).GetBooleanValue());
	
	Variables.Add("Male", ETextGender::Masculine);
	Variables.Add("Female", ETextGender::Feminine);
	Variables.Add("AlsoFemale", ETextGender::Feminine);
	Variables.Add("Neuter", ETextGender::Neuter);
	TestTrue("Comparisons", Expr.ParseFromString("{Male} == masculine", ""));
	TestTrue("Eval", Expr.Evaluate(Variables).GetBooleanValue());
	TestTrue("Comparisons", Expr.ParseFromString("{Male} == Masculine", ""));
	TestTrue("Eval", Expr.Evaluate(Variables).GetBooleanValue());
	TestTrue("Comparisons", Expr.ParseFromString("{Male} == feminine", ""));
	TestFalse("Eval", Expr.Evaluate(Variables).GetBooleanValue());
	TestTrue("Comparisons", Expr.ParseFromString("{Male} == Feminine", ""));
	TestFalse("Eval", Expr.Evaluate(Variables).GetBooleanValue());
	TestTrue("Comparisons", Expr.ParseFromString("{Female} == Feminine", ""));
	TestTrue("Eval", Expr.Evaluate(Variables).GetBooleanValue());
	TestTrue("Comparisons", Expr.ParseFromString("{Female} != feminine", ""));
	TestFalse("Eval", Expr.Evaluate(Variables).GetBooleanValue());
	TestTrue("Comparisons", Expr.ParseFromString("{Female} == {AlsoFemale}", ""));
	TestTrue("Eval", Expr.Evaluate(Variables).GetBooleanValue());
	TestTrue("Comparisons", Expr.ParseFromString("{Female} != {Neuter}", ""));
	TestTrue("Eval", Expr.Evaluate(Variables).GetBooleanValue());
	TestTrue("Comparisons", Expr.ParseFromString("{Neuter} == neuter", ""));
	TestTrue("Eval", Expr.Evaluate(Variables).GetBooleanValue());
	TestTrue("Comparisons", Expr.ParseFromString("{Neuter} == Neuter", ""));
	TestTrue("Eval", Expr.Evaluate(Variables).GetBooleanValue());
	

	
	return true;
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestBadExpressions,
								 "SUDSTest.TestBadExpressions",
								 EAutomationTestFlags::EditorContext |
								 EAutomationTestFlags::ClientContext |
								 EAutomationTestFlags::ProductFilter)



bool FTestBadExpressions::RunTest(const FString& Parameters)
{
	FSUDSExpression Expr;
	TMap<FName, FSUDSValue> Variables;

	AddExpectedError("bad expression", EAutomationExpectedErrorFlags::Contains, 3);
	AddExpectedError("mismatched parentheses", EAutomationExpectedErrorFlags::Contains, 2);
	
	TestFalse("Missing operand", Expr.ParseFromString(" + 1", ""));
	TestFalse("Missing operand", Expr.ParseFromString("1 * ", ""));
	TestFalse("Missing parenthesis", Expr.ParseFromString("(3 + 1", ""));
	TestFalse("Missing parenthesis", Expr.ParseFromString("3 + 1)", ""));
	TestFalse("Invalid symbol", Expr.ParseFromString("something + 1", ""));
	
	return true;
}



PRAGMA_ENABLE_OPTIMIZATION