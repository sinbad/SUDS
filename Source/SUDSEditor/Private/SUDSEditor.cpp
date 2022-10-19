// Copyright Epic Games, Inc. All Rights Reserved.

#include "SUDSEditor.h"
#include "antlr4-runtime.h"
#include "antlr_gen/ExpressionLexer.h"
#include "antlr_gen/ExpressionParser.h"

#define LOCTEXT_NAMESPACE "FSUDSModule"

void FSUDSEditorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	antlr4::ANTLRInputStream input("6*(2+3)");
    
	// Create a lexer from the input
	ExpressionLexer lexer(&input);
    
	// Create a token stream from the lexer
	antlr4::CommonTokenStream tokens(&lexer);
    
	// Create a parser from the token stream
	ExpressionParser parser(&tokens);    

	// Display the parse tree
	// Note: use UTF8_TO_TCHAR(c_str()) for cases where there will be UTF8
	FString Output(parser.expr()->toStringTree().c_str());
	UE_LOG(LogTemp, Log, TEXT("%s"), *Output);
}

void FSUDSEditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FSUDSEditorModule, SUDSEditor)