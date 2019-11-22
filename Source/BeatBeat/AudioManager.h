// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "SoundManager_Fmod.h"
#include <memory>
#include "UObject/NoExportTypes.h"

#include "AudioManager.generated.h"




UCLASS(Blueprintable)
class BEATBEAT_API UAudioManager : public UObject
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = FMOD)
	int32 InitialiseManager();

	UFUNCTION(BlueprintCallable, Category = FMOD)
	int32 InitialiseRecorder();

	UFUNCTION(BlueprintCallable, Category = FMOD)
	int32 StartRecording();

	UFUNCTION(BlueprintCallable, Category = FMOD)
	int32 StopRecording();

	UFUNCTION(BlueprintCallable, Category = FMOD)
	int32 PlayRecordedSound();

	UFUNCTION(BlueprintCallable, Category = FMOD)
	int32 PauseRecordedSound();

	UFUNCTION(BlueprintCallable, Category = FMOD)
	void GetPitch(float &pitch);

	UFUNCTION(BlueprintCallable, Category = Math)
	void SortArray(UPARAM(ref) TArray<float> &inArray);

	UAudioManager();
	~UAudioManager();

public:
	UFUNCTION(BlueprintCallable, Category = FMOD)
	void InitBeatDetector();

	UFUNCTION(BlueprintCallable, Category = FMOD)
	void GetBeat(TArray<float>& frequencyValues, TArray<float>& frequencyAverageValues, bool& isBass, bool& isLowM, float& time);

	UFUNCTION(BlueprintCallable, Category = FMOD)
	int32 PlaySong(int num);

	UFUNCTION(BlueprintCallable, Category = FMOD)
	int32 PlaySongByName(FString name, float volume);

	UFUNCTION(BlueprintCallable, Category = FMOD)
	void PauseSong(bool unPause);

public: /*FFT Spectrum visualisation*/
	UFUNCTION(BlueprintCallable, Category = FMOD)
	void Update();


	UFUNCTION(BlueprintCallable, Category = FMOD)
	int32 InitSpectrum_Linear(const int32 numBars);

	UFUNCTION(BlueprintCallable, Category = FMOD)
	int32 InitSpectrum_Log(const int32 numBars);

	UFUNCTION(BlueprintCallable, Category = FMOD)
	int32 InitTimeDomain(const int32 numBars);

	UFUNCTION(BlueprintCallable, Category = FMOD)
	void GetSpectrum_Linear(TArray<float>& frequencyValues, const int32 effectiveBars);

	UFUNCTION(BlueprintCallable, Category = FMOD)
	void GetSpectrum_Log(TArray<float>& frequencyValues, const int32 effectiveBars);

	UFUNCTION(BlueprintCallable, Category = FMOD)
	void GetTimeDomain(TArray<float>& amplitudeValues, const int32 effectiveBars);



private: /*FMOD set up*/
	std::unique_ptr<SoundManager_Fmod> _soundManager;
};