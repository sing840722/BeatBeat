// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "Components/AudioComponent.h"
#include "OutputBuffer.generated.h"
/**
 * 
 */
//#define YIN_SAMPLING_RATE 44100
#define YIN_SAMPLING_RATE 44100
#define YIN_THRESHOLD 0.15

UCLASS(Blueprintable)
class BEATBEAT_API UOutputBuffer : public UAudioComponent
{
	GENERATED_BODY()
	

public:
	//UOutputBuffer();
	UFUNCTION(BlueprintCallable, Category = "CustomDSP")
	void CustomDSP(float sample);

	UFUNCTION(BlueprintCallable, Category = "Yin")
	void YinPitchTracking();

	//UFUNCTION(BlueprintCallable, Category = "Yin")

	UFUNCTION(BlueprintCallable, Category = "Yin")
	void MakeAudio(TArray<float> inputSample, int sampleSize);
	
	

public:
	//void Yin_Init(int16_t bufferSize, float threshold);
	void Yin_Init(int bufferSize);
	float Yin_getPitch(TArray<float> buffer);
	float Yin_getProbability();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:
	//Functions
	void Yin_difference(TArray<float>& buffer);
	void Yin_cumulativeMeanNormalizedDifference();
	int Yin_absoluteThreshold();
	float Yin_parabolicInterpolation(int tauEstimate);

	float *audio = NULL;
	TArray<float> audioData;
	float previousPitch;


	void TestData();

private:
	struct {
		int bufferSize;			/**< Size of the audio buffer to be analysed */
		int halfBufferSize;		/**< Half the buffer length */
		float* yinBuffer;		/**< Buffer that stores the results of the intermediate processing steps of the algorithm */
		float probability;		/**< Probability that the pitch found is correct as a decimal (i.e 0.85 is 85%) */
		float threshold;		/**< Allowed uncertainty in the result as a decimal (i.e 0.15 is 15%) */
	};
	
};
