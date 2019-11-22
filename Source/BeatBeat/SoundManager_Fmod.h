// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include <string>
#include <vector>
#include "fmod_common.h"
#include "fmod.hpp"
#include "fmod_errors.h"
#include "fmod.h"
#include "fmod_output.h"
#include "fmod_dsp.h"
#include "fmod_dsp_effects.h"
#include "YinPitchDetection.h"

#include <string>
#include <vector>
#include <deque>


#define LATENCY_MS      (50) /* Some devices will require higher latency to avoid glitches */
#define DRIFT_MS        (1)
#define DEVICE_INDEX    (0)
#define WINDOW_SIZE		(1024)


DECLARE_LOG_CATEGORY_EXTERN(LogMyGame, Log, All);

typedef std::deque<std::vector<float> > FFTHistoryContainer;


class BEATBEAT_API SoundManager_Fmod {
public:
	SoundManager_Fmod();
	~SoundManager_Fmod();

	int Initialise();
	void PlaySound(float volume);
	void PauseSound(bool unPause = false);
	void Update();
	int InitialiseSpectrum_Linear(int maxBands);
	int InitialiseSpectrum_Log(int bandsPerOctave);
	int InitialiseTimeDomain(int numBars);
	void GetSpectrum_Linear(float* spectrum);
	void GetSpectrum_Log(float* spectrum);
	void GetTimeDomain(float* bars);

	int32 InitialiseRecorder();
	int32 InitialiseRecordBuffer();
	int32 StartRecordSound();
	int32 StopRecordSound();
	int32 PlayRecordedSound();
	int32 PauseRecordedSound();

	float GetPitch();

	void initializeBeatDetector();
	void getBeat(float* spectrum, float* averageSpectrum, bool& isBass, bool& isLowM, float& time);

	/*Load and play music*/
	int LoadSoundFromPath(std::string pathToFile);
	int LoadSoundFromMemory(char* memoryPtr, unsigned int memorySize);

private:
	YinPitchDetection* _yin;

	FMOD::System* _system;
	FMOD::Channel* _channel;
	FMOD::Sound* _sound;

	FMOD::DSP*	_dsp;
	FMOD::DSP* _myDSP;

	FMOD::DSP* _recorderFFT;

	float* buffer;

	std::vector<int> _numSamplesPerBar_linear;
	std::vector<int> _numSamplesPerBar_log;
	std::vector<int> _numSamplesPerBar_timeDomain;

	/*Error Check & Debug*/
	bool FMOD_ERROR_CHECK(FMOD_RESULT result);
	void DebugLog(FString str);
	void DebugLog(float f);
	FMOD_RESULT result;

	/*Recording*/
	int numDrivers;
	FMOD::Channel *channel;
	unsigned int samplesRecorded = 0;
	unsigned int samplesPlayed = 0;

	unsigned int driftThreshold;// = (nativeRate * DRIFT_MS) / 1000;       /* The point where we start compensating for drift */
	unsigned int desiredLatency;// = (nativeRate * LATENCY_MS) / 1000;     /* User specified latency */
	unsigned int adjustedLatency;// = desiredLatency;                      /* User specified latency adjusted for driver update granularity */
	int actualLatency;// = desiredLatency;                                 /* Latency measured once playback begins (smoothened for jitter) */
	unsigned int soundLength = 0;
	int nativeRate = 0;
	unsigned int recordPos = 0;
	/*static*/ unsigned int lastRecordPos = 0;
	/*static*/ unsigned int minRecordDelta;
	unsigned int playPos = 0;


	FMOD::Sound *recordedSound;

	/*Beat Detection*/
	//int _windowSize;
	float _samplingFrequency;

	int _FFThistory_MaxSize;
	std::vector<int> _beatDetector_bandLimits;

	FFTHistoryContainer _FFTHistory_linear;
	FFTHistoryContainer _FFTHistory_log;
	FFTHistoryContainer _FFTHistory_beatDetector;

	static void fillAverageSpectrum(float* averageSpectrum, int numBands, const FFTHistoryContainer& fftHistory);
	static void fillVarianceSpectrum(float* varianceSpectrum, int numBands, const FFTHistoryContainer& fftHistory, const float* averageSpectrum);
	static float beatThreshold(float variance);

};