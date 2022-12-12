#pragma once
#include "SUDSDialogue.h"
#include "SUDSScriptNode.h"
#include "SUDSScriptNodeText.h"
#include "Internationalization/StringTable.h"
#include "Internationalization/StringTableRegistry.h"

FORCEINLINE void TestDialogueText(FAutomationTestBase* T, const FString& NameForTest, USUDSDialogue* D, const FString& SpeakerID, const FString& Text)
{
	T->TestEqual(NameForTest, D->GetSpeakerID(), SpeakerID);
	T->TestEqual(NameForTest, D->GetText().ToString(), Text);
	
}

FORCEINLINE bool TestParsedText(FAutomationTestBase* T, const FString& NameForTest, const FSUDSParsedNode* Node, const FString& Speaker, const FString& Text)
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

FORCEINLINE bool TestArgValue(FAutomationTestBase* T, const FString& NameForTest, const FSUDSValue& Actual, int Expected)
{
	if (T->TestEqual(NameForTest, Actual.GetType(), ESUDSValueType::Int))
	{
		return T->TestEqual(NameForTest, Actual.GetIntValue(), Expected);
	}
	return false;
}
FORCEINLINE bool TestArgValue(FAutomationTestBase* T, const FString& NameForTest, const FSUDSValue& Actual, float Expected)
{
	if (T->TestEqual(NameForTest, Actual.GetType(), ESUDSValueType::Float))
	{
		return T->TestEqual(NameForTest, Actual.GetFloatValue(), Expected);
	}
	return false;
}
FORCEINLINE bool TestArgValue(FAutomationTestBase* T, const FString& NameForTest, const FSUDSValue& Actual, ETextGender Expected)
{
	if (T->TestEqual(NameForTest, Actual.GetType(), ESUDSValueType::Gender))
	{
		return T->TestEqual(NameForTest, Actual.GetGenderValue(), Expected);
	}
	return false;
}
FORCEINLINE bool TestArgValue(FAutomationTestBase* T, const FString& NameForTest, const FSUDSValue& Actual, const FString& Expected)
{
	if (T->TestEqual(NameForTest, Actual.GetType(), ESUDSValueType::Text))
	{
		return T->TestEqual(NameForTest, Actual.GetTextValue().ToString(), Expected);
	}
	return false;
}

template <typename V>
FORCEINLINE bool TestParsedSetLiteral(FAutomationTestBase* T, const FString& NameForTest, const FSUDSParsedNode* Node, const FString& VarName, V Literal)
{
	if (T->TestNotNull(NameForTest, Node))
	{
		T->TestEqual(NameForTest, Node->NodeType, ESUDSParsedNodeType::SetVariable);
		T->TestEqual(NameForTest, Node->Identifier, VarName);
		if (T->TestTrue(NameForTest, Node->Expression.IsLiteral()))
		{
			TestArgValue(T, NameForTest,Node->Expression.GetLiteralValue(), Literal);	
		}
		return true;
	}
	return false;
	
}

// Explicit bool version of the above since otherwise it gets converted to int & fails
FORCEINLINE bool TestParsedSetLiteral(FAutomationTestBase* T, const FString& NameForTest, const FSUDSParsedNode* Node, const FString& VarName, bool Literal)
{
	if (T->TestNotNull(NameForTest, Node))
	{
		T->TestEqual(NameForTest, Node->NodeType, ESUDSParsedNodeType::SetVariable);
		T->TestEqual(NameForTest, Node->Identifier, VarName);
		if (T->TestTrue(NameForTest, Node->Expression.IsLiteral()))
		{
			T->TestEqual(NameForTest,Node->Expression.GetBooleanLiteralValue(), Literal);	
		}
		
		return true;
	}
	return false;
	
}

FORCEINLINE bool TestParsedSetLiteral(FAutomationTestBase* T, const FString& NameForTest, const FSUDSParsedNode* Node, const FString& VarName, const FName& Name)
{
	if (T->TestNotNull(NameForTest, Node))
	{
		T->TestEqual(NameForTest, Node->NodeType, ESUDSParsedNodeType::SetVariable);
		T->TestEqual(NameForTest, Node->Identifier, VarName);
		if (T->TestTrue(NameForTest, Node->Expression.IsLiteral()))
		{
			T->TestEqual(NameForTest,Node->Expression.GetNameLiteralValue(), Name);	
		}
		
		return true;
	}
	return false;
	
}

FORCEINLINE bool TestGetParsedNextNode(FAutomationTestBase* T, const FString& NameForTest, const FSUDSParsedNode* Node, FSUDSScriptImporter& Importer, bool bIsHeader, const FSUDSParsedNode** OutNextNode)
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
	*OutNextNode = nullptr;
	return false;
}
FORCEINLINE bool TestParsedChoice(FAutomationTestBase* T, const FString& NameForTest, const FSUDSParsedNode* Node, int ExpectedNumChoices)
{
	if (T->TestNotNull(NameForTest, Node))
	{
		T->TestEqual(NameForTest, Node->NodeType, ESUDSParsedNodeType::Choice);
		T->TestEqual(NameForTest, Node->Edges.Num(), ExpectedNumChoices);
		return true;
	}
	return false;
}
FORCEINLINE bool TestParsedSelect(FAutomationTestBase* T, const FString& NameForTest, const FSUDSParsedNode* Node, int ExpectedNumEdges)
{
	if (T->TestNotNull(NameForTest, Node))
	{
		T->TestEqual(NameForTest, Node->NodeType, ESUDSParsedNodeType::Select);
		T->TestEqual(NameForTest, Node->Edges.Num(), ExpectedNumEdges);
		return true;
	}
	return false;
}

FORCEINLINE bool TestParsedChoiceEdge(FAutomationTestBase* T, const FString& NameForTest, const FSUDSParsedNode* Node, int EdgeIndex, const FString& Text, FSUDSScriptImporter& Importer, const FSUDSParsedNode** OutNode)
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

FORCEINLINE bool TestParsedSelectEdge(FAutomationTestBase* T, const FString& NameForTest, const FSUDSParsedNode* Node, int EdgeIndex, const FString& ConditionStr, FSUDSScriptImporter& Importer, const FSUDSParsedNode** OutNode)
{
	*OutNode = nullptr;
	if (Node && Node->Edges.Num() > EdgeIndex)
	{
		auto& Edge = Node->Edges[EdgeIndex];
		T->TestEqual(NameForTest, Edge.ConditionExpression.GetSourceString(), ConditionStr);
		const int Idx = Edge.TargetNodeIdx;
		*OutNode = Importer.GetNode(Idx);
		return true;
	}
	
	return false;
	
}

FORCEINLINE bool TestParsedGoto(FAutomationTestBase* T, const FString& NameForTest, const FSUDSParsedNode* Node, FSUDSScriptImporter& Importer, const FSUDSParsedNode** OutNode)
{
	if (T->TestNotNull(NameForTest, Node))
	{
		T->TestEqual(NameForTest, Node->NodeType, ESUDSParsedNodeType::Goto);
		const int Target = Importer.GetGotoTargetNodeIndex(Node->Identifier);
		*OutNode = Importer.GetNode(Target);
		return true;
	}
	return false;
}

FORCEINLINE bool TestTextNode(FAutomationTestBase* T, const FString& NameForTest, const USUDSScriptNode* Node, const FString& Speaker, const FString& Text)
{
	if (T->TestNotNull(NameForTest, Node))
	{
		T->TestEqual(NameForTest, Node->GetNodeType(), ESUDSScriptNodeType::Text);
		if (auto TextNode = Cast<USUDSScriptNodeText>(Node))
		{
			T->TestEqual(NameForTest, TextNode->GetSpeakerID(), Speaker);
			T->TestEqual(NameForTest, TextNode->GetText().ToString(), Text);
		}
		return true;
	}
	return false;
}

FORCEINLINE bool TestEdge(FAutomationTestBase* T, const FString& NameForTest, USUDSScriptNode* Node, int EdgeIndex, USUDSScriptNode** OutNode)
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

FORCEINLINE bool TestChoiceNode(FAutomationTestBase* T, const FString& NameForTest, const USUDSScriptNode* Node, int NumChoices)
{
	if (T->TestNotNull(NameForTest, Node))
	{
		T->TestEqual(NameForTest, Node->GetNodeType(), ESUDSScriptNodeType::Choice);
		return T->TestEqual(NameForTest, Node->GetEdgeCount(), NumChoices);
	}
	return false;
}

FORCEINLINE bool TestChoiceEdge(FAutomationTestBase* T, const FString& NameForTest, USUDSScriptNode* Node, int EdgeIndex, const FString& Text, USUDSScriptNode** OutNode)
{
	*OutNode = nullptr;
	if (Node && Node->GetEdgeCount() > EdgeIndex)
	{
		if (auto Edge = Node->GetEdge(EdgeIndex))
		{
			T->TestEqual(NameForTest, Edge->GetText().ToString(), Text);
			*OutNode = Edge->GetTargetNode().Get();
			return T->TestNotNull(NameForTest, *OutNode);
		}
	}
	
	return false;
	
}

// Helper to provide a string table just in scope
struct ScopedStringTableHolder
{
public:

	UStringTable* StringTable;
	
	ScopedStringTableHolder()
	{
		StringTable = NewObject<UStringTable>(GetTransientPackage(), "TestStrings");
	}

	~ScopedStringTableHolder()
	{
		// Tidy up string table
		// UStringTable constructor registered this table
		FStringTableRegistry::Get().UnregisterStringTable(StringTable->GetStringTableId());
	}
	
};
