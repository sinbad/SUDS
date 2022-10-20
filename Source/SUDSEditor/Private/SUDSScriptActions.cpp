

#include "SUDSScriptActions.h"

#include "SUDSScript.h"

FText FSUDSScriptActions::GetName() const
{
	return INVTEXT("SUDS Script");
}

FString FSUDSScriptActions::GetObjectDisplayName(UObject* Object) const
{
	return TEXT("SUDS Script Asset");
}

UClass* FSUDSScriptActions::GetSupportedClass() const
{
	return USUDSScript::StaticClass();
}

FColor FSUDSScriptActions::GetTypeColor() const
{
	return FColor::Orange;
}

uint32 FSUDSScriptActions::GetCategories()
{
	return EAssetTypeCategories::Misc;
}
