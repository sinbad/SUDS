// Copyright Epic Games, Inc. All Rights Reserved.

#include "SUDSEditor.h"

#define LOCTEXT_NAMESPACE "FSUDSModule"

void FSUDSEditorModule::StartupModule()
{
}

void FSUDSEditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FSUDSEditorModule, SUDSEditor)