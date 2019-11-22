// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
#define YIN_SAMPLING_RATE (44100)
#define YIN_THRESHOLD (0.1)

class BEATBEAT_API YinPitchDetection
{
public:
	YinPitchDetection();
	~YinPitchDetection();

	float YinPitchTracking(float *audioData);
	void Yin_Init(int bufferSize);
	float Yin_getPitch(float* audioData);
	float Yin_getProbability();

private:
	void Yin_difference(float* audioData);
	void Yin_cumulativeMeanNormalizedDifference();
	int Yin_absoluteThreshold();
	float Yin_parabolicInterpolation(int tauEstimate);


	int bufferSize;			/**< Size of the audio buffer to be analysed */
	int halfBufferSize;		/**< Half the buffer length */
	float* yinBuffer;		/**< Buffer that stores the results of the intermediate processing steps of the algorithm */
	float probability;		/**< Probability that the pitch found is correct as a decimal (i.e 0.85 is 85%) */
	float threshold;		/**< Allowed uncertainty in the result as a decimal (i.e 0.15 is 15%) */

};
