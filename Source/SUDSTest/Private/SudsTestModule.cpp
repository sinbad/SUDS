#include "SudsTestModule.h"

DEFINE_LOG_CATEGORY(LogSudsTestModule)

void FSudsTestModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	UE_LOG(LogSudsTestModule, Log, TEXT("SUDSTest Module Started"))
}

void FSudsTestModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	UE_LOG(LogSudsTestModule, Log, TEXT("SUDSTest Module Stopped"))
}


IMPLEMENT_MODULE(FSudsTestModule, SUDSTest)
