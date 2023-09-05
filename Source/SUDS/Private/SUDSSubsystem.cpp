// Copyright Steve Streeting 2022
// Released under the MIT license https://opensource.org/license/MIT/
#include "SUDSSubsystem.h"
#include "Sound/SoundConcurrency.h"

DEFINE_LOG_CATEGORY(LogSUDSSubsystem)

void USUDSSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Default to a single voice line being played at once
	VoiceConcurrency = NewObject<USoundConcurrency>(this);
	VoiceConcurrency->Concurrency.MaxCount = 1;
}

void USUDSSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void USUDSSubsystem::SetMaxConcurrentVoicedLines(int ConcurrentLines)
{
	if (IsValid(VoiceConcurrency))
	{
		VoiceConcurrency->Concurrency.MaxCount = ConcurrentLines;
	}
}

int USUDSSubsystem::GetMaxConcurrentVoicedLines() const
{
	if (IsValid(VoiceConcurrency))
	{
		return VoiceConcurrency->Concurrency.MaxCount;
	}
	return 1;
}

void USUDSSubsystem::SetVoicedLineConcurrencySettings(const FSoundConcurrencySettings& NewSettings)
{
	if (IsValid(VoiceConcurrency))
	{
		VoiceConcurrency->Concurrency = NewSettings;
	}
}

const FSoundConcurrencySettings& USUDSSubsystem::GetVoicedLineConcurrencySettings() const
{
	if (IsValid(VoiceConcurrency))
	{
		return VoiceConcurrency->Concurrency;
	}
	
	static const FSoundConcurrencySettings Dummy;
	return Dummy;
	
}
