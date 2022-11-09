#pragma once
#include "SUDSDialogue.h"
#include "SUDSScriptNode.h"

inline void TestDialogueText(FAutomationTestBase* T, const FString& NameForTest, USUDSDialogue* D, const FString& SpeakerID, const FString& Text)
{
	T->TestEqual(NameForTest, D->GetSpeakerID(), SpeakerID);
	T->TestEqual(NameForTest, D->GetText().ToString(), Text);
	
}

inline bool TestParsedText(FAutomationTestBase* T, const FString& NameForTest, const FSUDSParsedNode* Node, const FString& Speaker, const FString& Text)
{
	if (T->TestNotNull(NameForTest, Node))
	{
		T->TestEqual(NameForTest, Node->NodeType, ESUDSParsedNodeType::Text);
		T->TestEqual(NameForTest, Node->Identifier, Speaker);
		T->TestEqual(NameForTest, Node->Text, Text);
		return true;
	}
	return false;
}

inline bool TestArgValue(FAutomationTestBase* T, const FString& NameForTest, const FFormatArgumentValue& Actual, int Expected)
{
	if (T->TestEqual(NameForTest, Actual.GetType(), EFormatArgumentType::Int))
	{
		return T->TestEqual(NameForTest, Actual.GetIntValue(), Expected);
	}
	return false;
}
inline bool TestArgValue(FAutomationTestBase* T, const FString& NameForTest, const FFormatArgumentValue& Actual, float Expected)
{
	if (T->TestEqual(NameForTest, Actual.GetType(), EFormatArgumentType::Float))
	{
		return T->TestEqual(NameForTest, Actual.GetFloatValue(), Expected);
	}
	return false;
}
inline bool TestArgValue(FAutomationTestBase* T, const FString& NameForTest, const FFormatArgumentValue& Actual, ETextGender Expected)
{
	if (T->TestEqual(NameForTest, Actual.GetType(), EFormatArgumentType::Gender))
	{
		return T->TestEqual(NameForTest, Actual.GetGenderValue(), Expected);
	}
	return false;
}
inline bool TestArgValue(FAutomationTestBase* T, const FString& NameForTest, const FFormatArgumentValue& Actual, const FString& Expected)
{
	if (T->TestEqual(NameForTest, Actual.GetType(), EFormatArgumentType::Text))
	{
		return T->TestEqual(NameForTest, Actual.GetTextValue().ToString(), Expected);
	}
	return false;
}

template <typename V>
inline bool TestParsedSetLiteral(FAutomationTestBase* T, const FString& NameForTest, const FSUDSParsedNode* Node, const FString& VarName, V Literal)
{
	if (T->TestNotNull(NameForTest, Node))
	{
		T->TestEqual(NameForTest, Node->NodeType, ESUDSParsedNodeType::SetVariable);
		T->TestEqual(NameForTest, Node->Identifier, VarName);
		TestArgValue(T, NameForTest,Node->VarLiteral, Literal);
		return true;
	}
	return false;
	
}

inline bool TestTextNode(FAutomationTestBase* T, const FString& NameForTest, const USUDSScriptNode* Node, const FString& Speaker, const FString& Text)
{
	if (T->TestNotNull(NameForTest, Node))
	{
		T->TestEqual(NameForTest, Node->GetNodeType(), ESUDSScriptNodeType::Text);
		T->TestEqual(NameForTest, Node->GetSpeakerID(), Speaker);
		T->TestEqual(NameForTest, Node->GetText().ToString(), Text);
		return true;
	}
	return false;
}

inline bool TestEdge(FAutomationTestBase* T, const FString& NameForTest, USUDSScriptNode* Node, int EdgeIndex, USUDSScriptNode** OutNode)
{
	*OutNode = nullptr;
	if (Node && Node->GetEdgeCount() > EdgeIndex)
	{
		if (auto Edge = Node->GetEdge(EdgeIndex))
		{
			*OutNode = Edge->GetTargetNode().Get();
			return T->TestNotNull(NameForTest, *OutNode);
		}
	}
	
	return false;
	
}

inline bool TestChoiceNode(FAutomationTestBase* T, const FString& NameForTest, const USUDSScriptNode* Node, int NumChoices)
{
	if (T->TestNotNull(NameForTest, Node))
	{
		T->TestEqual(NameForTest, Node->GetNodeType(), ESUDSScriptNodeType::Choice);
		return T->TestEqual(NameForTest, Node->GetEdgeCount(), NumChoices);
	}
	return false;
}

inline bool TestChoiceEdge(FAutomationTestBase* T, const FString& NameForTest, USUDSScriptNode* Node, int EdgeIndex, const FString& Text, USUDSScriptNode** OutNode)
{
	*OutNode = nullptr;
	if (Node && Node->GetEdgeCount() > EdgeIndex)
	{
		if (auto Edge = Node->GetEdge(EdgeIndex))
		{
			T->TestEqual(NameForTest, Edge->GetNavigation(), ESUDSScriptEdgeNavigation::Explicit);
			T->TestEqual(NameForTest, Edge->GetText().ToString(), Text);
			*OutNode = Edge->GetTargetNode().Get();
			return T->TestNotNull(NameForTest, *OutNode);
		}
	}
	
	return false;
	
}
