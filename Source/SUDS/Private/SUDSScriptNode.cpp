// Copyright 2020 Old Doorways Ltd


#include "SUDSScriptNode.h"

USUDSScriptNode::USUDSScriptNode()
{
}

USUDSScriptNode::USUDSScriptNode(ESUDSScriptNodeType Typ)
	: NodeType(Typ)
{
}

USUDSScriptNode::USUDSScriptNode(const FString& InTextID)
	: NodeType(ESUDSScriptNodeType::Text), TextID(InTextID)

{
}

void USUDSScriptNode::AddEdge(const FSUDSScriptEdge& NewEdge)
{
	Edges.Add(NewEdge);
	MarkPackageDirty();
}
