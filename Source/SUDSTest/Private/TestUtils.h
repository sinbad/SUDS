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

inline bool TestGetParsedNextNode(FAutomationTestBase* T, const FString& NameForTest, const FSUDSParsedNode* Node, FSUDSScriptImporter& Importer, bool bIsHeader, const FSUDSParsedNode** OutNextNode)
{
	if (T->TestNotNull(NameForTest, Node))
	{
		if (T->TestEqual(NameForTest, Node->Edges.Num(), 1))
		{
			const int NextNodeIdx = Node->Edges[0].TargetNodeIdx;
			*OutNextNode = bIsHeader ? Importer.GetHeaderNode(NextNodeIdx) : Importer.GetNode(NextNodeIdx); 
			return true;
		}
	}
	return false;
}
inline bool TestParsedChoice(FAutomationTestBase* T, const FString& NameForTest, const FSUDSParsedNode* Node, int ExpectedNumChoices)
{
	if (T->TestNotNull(NameForTest, Node))
	{
		T->TestEqual(NameForTest, Node->NodeType, ESUDSParsedNodeType::Choice);
		T->TestEqual(NameForTest, Node->Edges.Num(), ExpectedNumChoices);
		return true;
	}
	return false;
}

inline bool TestParsedChoiceEdge(FAutomationTestBase* T, const FString& NameForTest, const FSUDSParsedNode* Node, int EdgeIndex, const FString& Text, FSUDSScriptImporter& Importer, const FSUDSParsedNode** OutNode)
{
	*OutNode = nullptr;
	if (Node && Node->Edges.Num() > EdgeIndex)
	{
		auto& Edge = Node->Edges[EdgeIndex];
		T->TestEqual(NameForTest, Edge.Text, Text);
		const int Idx = Edge.TargetNodeIdx;
		*OutNode = Importer.GetNode(Idx);
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
