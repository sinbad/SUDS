#pragma once

#include "CoreMinimal.h"
#include "SUDSScriptImporter.h"
#include "Factories/Factory.h"
#include "SUDSScriptFactory.generated.h"


/**
 * 
 */
UCLASS()
class SUDSEDITOR_API USUDSScriptFactory : public UFactory
{
	GENERATED_BODY()

public:
	USUDSScriptFactory();
protected:
	virtual UObject* FactoryCreateText(UClass* InClass,
		UObject* InParent,
		FName InName,
		EObjectFlags Flags,
		UObject* Context,
		const TCHAR* Type,
		const TCHAR*& Buffer,
		const TCHAR* BufferEnd,
		FFeedbackContext* Warn) override;

	FSUDSScriptImporter Importer;
};
