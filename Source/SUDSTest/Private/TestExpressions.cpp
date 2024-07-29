#include "SUDSExpression.h"
#include "Misc/AutomationTest.h"

UE_DISABLE_OPTIMIZATION

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTestExpressions,
								 "SUDSTest.TestExpressions",
								 EAutomationTestFlags::EditorContext |
								 EAutomationTestFlags::ClientContext |
								 EAutomationTestFlags::ProductFilter)



bool FTestExpressions::RunTest(const FString& Parameters)
{
	FSUDSExpression Expr;
	TMap<FName, FSUDSValue> Variables;
	TMap<FName, FSUDSValue> GlobalVariables;

	Variables.Add("Six", 6);
	TestTrue("SimpleVarParse", Expr.ParseFromString("3 + 4 * {Six} + 1", nullptr));
	TestEqual("Eval", Expr.Evaluate(Variables, GlobalVariables).GetIntValue(), 28);
	
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

	TestTrue("Arithmetic", Expr.ParseFromString("-6.7 * 2 + (21.3 - 8) * 5", nullptr));
	TestEqual("Eval", Expr.Evaluate(Variables, GlobalVariables).GetFloatValue(), 53.1f);

	// Modulo operator
	TestTrue("ModuloIntOperator", Expr.ParseFromString("11 % 5", nullptr));
	TestEqual("Eval", Expr.Evaluate(Variables, GlobalVariables).GetIntValue(), 1);
	TestTrue("ModuloFloatOperator", Expr.ParseFromString("7.25 % 3.0", nullptr));
	TestEqual("Eval", Expr.Evaluate(Variables, GlobalVariables).GetFloatValue(), 1.25f);
	
	// Explicit FSUDSValue(true) needed to avoid it using the int conversion by default
	Variables.Add("IsATest", FSUDSValue(true));
	TestTrue("BoolSingleValueParse", Expr.ParseFromString("{IsATest}", nullptr));
	TestTrue("Eval", Expr.Evaluate(Variables, GlobalVariables).GetBooleanValue());
	if (TestEqual("Variable count", Expr.GetVariableNames().Num(), 1))
	{
		TestEqual("Variable name", Expr.GetVariableNames()[0].ToString(), "IsATest");
	}
	
	Variables.Add("SomethingFalse", FSUDSValue(false));
	Variables.Add("SomethingTrue", FSUDSValue(true));
	Variables.Add("SomethingElseFalse", FSUDSValue(false));
	TestTrue("BoolCompound1", Expr.ParseFromString("!{SomethingFalse} && {SomethingTrue}", nullptr));
	TestTrue("Eval", Expr.Evaluate(Variables, GlobalVariables).GetBooleanValue());
	if (TestEqual("Variable count", Expr.GetVariableNames().Num(), 2))
	{
		TestEqual("Variable name", Expr.GetVariableNames()[0].ToString(), "SomethingFalse");
		TestEqual("Variable name", Expr.GetVariableNames()[1].ToString(), "SomethingTrue");
	}
	TestTrue("BoolCompound2", Expr.ParseFromString("{SomethingFalse} || {SomethingTrue}", nullptr));
	TestTrue("Eval", Expr.Evaluate(Variables, GlobalVariables).GetBooleanValue());
	TestTrue("BoolCompound3", Expr.ParseFromString("{SomethingFalse} or {SomethingTrue}", nullptr));
	TestTrue("Eval", Expr.Evaluate(Variables, GlobalVariables).GetBooleanValue());
	// Test parentheses changing precedence & result
	// True result for successful parsing, but false for Eval unless we parenthesise
	TestTrue("BoolCompound4", Expr.ParseFromString("!{SomethingFalse} && {SomethingElseFalse} && {SomethingTrue}", nullptr));
	TestFalse("Eval", Expr.Evaluate(Variables, GlobalVariables).GetBooleanValue());
	if (TestEqual("Variable count", Expr.GetVariableNames().Num(), 3))
	{
		TestEqual("Variable name", Expr.GetVariableNames()[0].ToString(), "SomethingFalse");
		TestEqual("Variable name", Expr.GetVariableNames()[1].ToString(), "SomethingElseFalse");
		TestEqual("Variable name", Expr.GetVariableNames()[2].ToString(), "SomethingTrue");
	}
	TestTrue("BoolCompound5", Expr.ParseFromString("!({SomethingFalse} && {SomethingElseFalse}) && {SomethingTrue}", nullptr));
	TestTrue("Eval", Expr.Evaluate(Variables, GlobalVariables).GetBooleanValue());
	TestTrue("BoolCompound6", Expr.ParseFromString("not {SomethingFalse} and {SomethingElseFalse} and {SomethingTrue}", nullptr));
	TestFalse("Eval", Expr.Evaluate(Variables, GlobalVariables).GetBooleanValue());
	TestTrue("BoolCompound7", Expr.ParseFromString("not ({SomethingFalse} and {SomethingElseFalse}) and {SomethingTrue}", nullptr));
	TestTrue("Eval", Expr.Evaluate(Variables, GlobalVariables).GetBooleanValue());

	Variables.Add("Seven", 7);
	TestTrue("Comparisons", Expr.ParseFromString("{Six} == 6", nullptr));
	TestTrue("Eval", Expr.Evaluate(Variables, GlobalVariables).GetBooleanValue());
	TestTrue("Comparisons", Expr.ParseFromString("{Six} = 6", nullptr));
	TestTrue("Eval", Expr.Evaluate(Variables, GlobalVariables).GetBooleanValue());
	TestTrue("Comparisons", Expr.ParseFromString("{Six} >= 6", nullptr));
	TestTrue("Eval", Expr.Evaluate(Variables, GlobalVariables).GetBooleanValue());
	TestTrue("Comparisons", Expr.ParseFromString("{Six} > 6", nullptr));
	TestFalse("Eval", Expr.Evaluate(Variables, GlobalVariables).GetBooleanValue());
	TestTrue("Comparisons", Expr.ParseFromString("{Six} < 6", nullptr));
	TestFalse("Eval", Expr.Evaluate(Variables, GlobalVariables).GetBooleanValue());
	TestTrue("Comparisons", Expr.ParseFromString("{Six} <= 6", nullptr));
	TestTrue("Eval", Expr.Evaluate(Variables, GlobalVariables).GetBooleanValue());
	TestTrue("Comparisons", Expr.ParseFromString("{Six} < {Seven}", nullptr));
	TestTrue("Eval", Expr.Evaluate(Variables, GlobalVariables).GetBooleanValue());
	TestTrue("Comparisons", Expr.ParseFromString("{Seven} > {Six}", nullptr));
	TestTrue("Eval", Expr.Evaluate(Variables, GlobalVariables).GetBooleanValue());
	TestTrue("Comparisons", Expr.ParseFromString("{Seven} != {Six}", nullptr));
	TestTrue("Eval", Expr.Evaluate(Variables, GlobalVariables).GetBooleanValue());
	TestTrue("Comparisons", Expr.ParseFromString("{Seven} != {Seven}", nullptr));
	TestFalse("Eval", Expr.Evaluate(Variables, GlobalVariables).GetBooleanValue());

	// Mixed float/int comparisons
	Variables.Add("EightFloat", 8.1f);
	TestTrue("Comparisons", Expr.ParseFromString("{EightFloat} > 8", nullptr));
	TestTrue("Eval", Expr.Evaluate(Variables, GlobalVariables).GetBooleanValue());
	TestTrue("Comparisons", Expr.ParseFromString("{EightFloat} > {Seven}", nullptr));
	TestTrue("Eval", Expr.Evaluate(Variables, GlobalVariables).GetBooleanValue());
	TestTrue("Comparisons", Expr.ParseFromString("{Six} < {EightFloat}", nullptr));
	TestTrue("Eval", Expr.Evaluate(Variables, GlobalVariables).GetBooleanValue());
	// Fuzzy float comparisons
	Variables.Add("EightFloatPlusMargin", 8.1000002f);
	TestTrue("Comparisons", Expr.ParseFromString("{EightFloat} == 8.1", nullptr));
	TestTrue("Eval", Expr.Evaluate(Variables, GlobalVariables).GetBooleanValue());
	TestTrue("Comparisons", Expr.ParseFromString("{EightFloat} == 8.15", nullptr));
	TestFalse("Eval", Expr.Evaluate(Variables, GlobalVariables).GetBooleanValue());
	TestTrue("Comparisons", Expr.ParseFromString("{EightFloatPlusMargin} == 8.1", nullptr));
	TestTrue("Eval", Expr.Evaluate(Variables, GlobalVariables).GetBooleanValue());
	TestTrue("Comparisons", Expr.ParseFromString("{EightFloat} == 8.1000005", nullptr));
	TestTrue("Eval", Expr.Evaluate(Variables, GlobalVariables).GetBooleanValue());
	TestTrue("Comparisons", Expr.ParseFromString("{EightFloatPlusMargin} == 8.1000005", nullptr));
	TestTrue("Eval", Expr.Evaluate(Variables, GlobalVariables).GetBooleanValue());

	// Other type comparisons
	Variables.Add("SomeText", FText::FromString("Hello"));
	TestTrue("Comparisons", Expr.ParseFromString("{SomeText} == \"Hello\"", nullptr));
	TestTrue("Eval", Expr.Evaluate(Variables, GlobalVariables).GetBooleanValue());
	TestTrue("Comparisons", Expr.ParseFromString("{SomeText} == \"Hi\"", nullptr));
	TestFalse("Eval", Expr.Evaluate(Variables, GlobalVariables).GetBooleanValue());
	
	Variables.Add("Male", ETextGender::Masculine);
	Variables.Add("Female", ETextGender::Feminine);
	Variables.Add("AlsoFemale", ETextGender::Feminine);
	Variables.Add("Neuter", ETextGender::Neuter);
	TestTrue("Comparisons", Expr.ParseFromString("{Male} == masculine", nullptr));
	TestTrue("Eval", Expr.Evaluate(Variables, GlobalVariables).GetBooleanValue());
	TestTrue("Comparisons", Expr.ParseFromString("{Male} == Masculine", nullptr));
	TestTrue("Eval", Expr.Evaluate(Variables, GlobalVariables).GetBooleanValue());
	TestTrue("Comparisons", Expr.ParseFromString("{Male} == feminine", nullptr));
	TestFalse("Eval", Expr.Evaluate(Variables, GlobalVariables).GetBooleanValue());
	TestTrue("Comparisons", Expr.ParseFromString("{Male} == Feminine", nullptr));
	TestFalse("Eval", Expr.Evaluate(Variables, GlobalVariables).GetBooleanValue());
	TestTrue("Comparisons", Expr.ParseFromString("{Female} == Feminine", nullptr));
	TestTrue("Eval", Expr.Evaluate(Variables, GlobalVariables).GetBooleanValue());
	TestTrue("Comparisons", Expr.ParseFromString("{Female} != feminine", nullptr));
	TestFalse("Eval", Expr.Evaluate(Variables, GlobalVariables).GetBooleanValue());
	TestTrue("Comparisons", Expr.ParseFromString("{Female} == {AlsoFemale}", nullptr));
	TestTrue("Eval", Expr.Evaluate(Variables, GlobalVariables).GetBooleanValue());
	TestTrue("Comparisons", Expr.ParseFromString("{Female} != {Neuter}", nullptr));
	TestTrue("Eval", Expr.Evaluate(Variables, GlobalVariables).GetBooleanValue());
	TestTrue("Comparisons", Expr.ParseFromString("{Neuter} == neuter", nullptr));
	TestTrue("Eval", Expr.Evaluate(Variables, GlobalVariables).GetBooleanValue());
	TestTrue("Comparisons", Expr.ParseFromString("{Neuter} == Neuter", nullptr));
	TestTrue("Eval", Expr.Evaluate(Variables, GlobalVariables).GetBooleanValue());

	// Test local / global by defining the same variable
	GlobalVariables.Add("GlobalLocalTestInt", 3);
	Variables.Add("GlobalLocalTestInt", 20);
	TestTrue("LocalTest", Expr.ParseFromString("{GlobalLocalTestInt} == 20", nullptr));
	TestTrue("Eval", Expr.Evaluate(Variables, GlobalVariables).GetBooleanValue());
	TestTrue("GlobalTest", Expr.ParseFromString("{global.GlobalLocalTestInt} == 3", nullptr));
	TestTrue("Eval", Expr.Evaluate(Variables, GlobalVariables).GetBooleanValue());
	

	
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

	FString ParseError;
	
	TestFalse("Missing operand", Expr.ParseFromString(" + 1", &ParseError));
	TestTrue("Correct error", ParseError.Contains("Bad expression"));
	TestFalse("Missing operand", Expr.ParseFromString("1 * ", &ParseError));
	TestTrue("Correct error", ParseError.Contains("Bad expression"));
	TestFalse("Missing parenthesis", Expr.ParseFromString("(3 + 1", &ParseError));
	TestTrue("Correct error", ParseError.Contains("Mismatched parentheses"));
	TestFalse("Missing parenthesis", Expr.ParseFromString("3 + 1)", &ParseError));
	TestTrue("Correct error", ParseError.Contains("Mismatched parentheses"));
	TestFalse("Invalid symbol", Expr.ParseFromString("something + 1", &ParseError));
	TestTrue("Correct error", ParseError.Contains("Bad expression"));
	
	return true;
}



UE_ENABLE_OPTIMIZATION