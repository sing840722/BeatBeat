// Fill out your copyright notice in the Description page of Project Settings.

#include "YinPitchDetection.h"

YinPitchDetection::YinPitchDetection()
{
}

YinPitchDetection::~YinPitchDetection()
{
}

float YinPitchDetection::YinPitchTracking(float* audioData) {
	float pitch = 0;
	float periodicity = 0;

	Yin_Init(2048);
	pitch = Yin_getPitch(audioData);

	if (Yin_getProbability() > 0.9)
		return pitch;
	else
		return -1;
}

void YinPitchDetection::Yin_Init(int bufferSize) {
	this->bufferSize = bufferSize;
	this->halfBufferSize = bufferSize / 2;
	this->probability = 0.0f;
	this->threshold = YIN_THRESHOLD;

	this->yinBuffer = (float *)malloc(sizeof(float)* this->halfBufferSize);
	for (int i = 0; i < this->halfBufferSize; i++) {
		this->yinBuffer[i] = 0;
	}
}

float YinPitchDetection::Yin_getPitch(float* buffer) {
	int tauEstimate = -1;
	float pitchInHertz = -1;

	/* Step 1: Calculates the squared difference of the signal with a shifted version of itself. */
	Yin_difference(buffer);

	/* Step 2: Calculate the cumulative mean on the normalised difference calculated in step 1 */
	Yin_cumulativeMeanNormalizedDifference();

	/* Step 3: Search through the normalised cumulative mean array and find values that are over the threshold */
	tauEstimate = Yin_absoluteThreshold();

	/* Step 5: Interpolate the shift value (tau) to improve the pitch estimate. */
	if (tauEstimate != -1) {
		pitchInHertz = YIN_SAMPLING_RATE / Yin_parabolicInterpolation(tauEstimate);
	}

	return pitchInHertz;
}


float YinPitchDetection::Yin_getProbability() {
	return this->probability;
}

/*Internal Functions*/
void YinPitchDetection::Yin_difference(float *audioData) {
	int i, tau;
	float delta;

	/* Calculate the difference for difference shift values (tau) for the half of the samples */
	for (tau = 0; tau < this->halfBufferSize; tau++) {
		/* Take the difference of the signal with a shifted version of itself, then square it.
		* (This is the Yin algorithm's tweak on autocorellation) */
		for (i = 0; i < this->halfBufferSize; i++) {
			delta = audioData[i] - audioData[i + tau];
			this->yinBuffer[tau] += delta * delta;
			//FString sPitch = FString::SanitizeFloat(this->yinBuffer[tau]);
			//GEngine->AddOnScreenDebugMessage(-1, 10.0, FColor::Red, *sPitch);
		}
	}
}

void YinPitchDetection::Yin_cumulativeMeanNormalizedDifference() {
	int tau;
	float runningSum = 0;
	this->yinBuffer[0] = 1;

	/* Sum all the values in the autocorellation buffer and nomalise the result, replacing
	* the value in the autocorellation buffer with a cumulative mean of the normalised difference */
	for (tau = 1; tau < this->halfBufferSize; tau++) {
		runningSum += this->yinBuffer[tau];
		this->yinBuffer[tau] *= tau / runningSum;
	}
}

int YinPitchDetection::Yin_absoluteThreshold() {
	int tau;

	/* Search through the array of cumulative mean values, and look for ones that are over the threshold
	* The first two positions in yinBuffer are always so start at the third (index 2) */
	for (tau = 2; tau < this->halfBufferSize; tau++) {

		if (yinBuffer[tau] < 0.9) {
			FString str2 = "The value in ";
			str2.Append(FString::SanitizeFloat(tau));
			str2.Append(" is ");
			str2.Append(FString::SanitizeFloat(yinBuffer[tau]));
			str2.Append("\n");
			//GEngine->AddOnScreenDebugMessage(-1, 10.0, FColor::Red, *str2);
		}

		if (this->yinBuffer[tau] < this->threshold) {
			while (tau + 1 < this->halfBufferSize && this->yinBuffer[tau + 1] < this->yinBuffer[tau]) {
				FString str3 = "While Loop: ";
				str3.Append(FString::SanitizeFloat(tau));
				str3.Append("\n");
				//GEngine->AddOnScreenDebugMessage(-1, 10.0, FColor::Red, *str3);
				tau++;
			}
			/* found tau, exit loop and return
			* store the probability
			* From the YIN paper: The yin->threshold determines the list of
			* candidates admitted to the set, and can be interpreted as the
			* proportion of aperiodic power tolerated
			* within a periodic signal.
			*
			* Since we want the periodicity and and not aperiodicity:
			* periodicity = 1 - aperiodicity */
			this->probability = 1 - this->yinBuffer[tau];
			break;
		}
	}
	/*
	FString str = "tau is found to be: ";
	FString sTau = FString::SanitizeFloat(tau);
	FString sValueAtTau = FString::SanitizeFloat(yinBuffer[tau]);
	FString sBufferSize = FString::SanitizeFloat(this->halfBufferSize);

	str.Append(sTau);
	str.Append(" with value: ");
	str.Append(sValueAtTau);
	str.Append(". The Yin Buffer Size is: ");
	str.Append(sBufferSize);
	str.Append("\n");

	GEngine->AddOnScreenDebugMessage(-1, 10.0, FColor::Red, *str);
	*/
	/* if no pitch found, tau => -1 */
	if (tau == this->halfBufferSize || this->yinBuffer[tau] >= this->threshold) {
		tau = -1;
		this->probability = 0;
	}



	return tau;
}

float YinPitchDetection::Yin_parabolicInterpolation(int tauEstimate) {
	float betterTau;
	int x0, x2;

	/* Calculate the first polynomial coeffcient based on the current estimate of tau */
	if (tauEstimate < 1) {
		x0 = tauEstimate;
	}
	else {
		x0 = tauEstimate - 1;
	}

	/* Calculate the second polynomial coeffcient based on the current estimate of tau */
	if (tauEstimate + 1 < this->halfBufferSize) {
		x2 = tauEstimate + 1;
	}
	else {
		x2 = tauEstimate;
	}

	/* Algorithm to parabolically interpolate the shift value tau to find a better estimate */
	if (x0 == tauEstimate) {
		if (this->yinBuffer[tauEstimate] <= this->yinBuffer[x2]) {
			betterTau = tauEstimate;
		}
		else {
			betterTau = x2;
		}
	}
	else if (x2 == tauEstimate) {
		if (this->yinBuffer[tauEstimate] <= this->yinBuffer[x0]) {
			betterTau = tauEstimate;
		}
		else {
			betterTau = x0;
		}
	}
	else {
		float s0, s1, s2;
		s0 = this->yinBuffer[x0];
		s1 = this->yinBuffer[tauEstimate];
		s2 = this->yinBuffer[x2];
		// fixed AUBIO implementation, thanks to Karl Helgason:
		// (2.0f * s1 - s2 - s0) was incorrectly multiplied with -1
		betterTau = tauEstimate + (s2 - s0) / (2 * (2 * s1 - s2 - s0));
	}


	return betterTau;
}
