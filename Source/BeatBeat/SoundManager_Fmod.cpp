#include "SoundManager_Fmod.h"
#include "fmod.hpp"
#include "fmod_common.h"
#include <string>
#include <vector>
using namespace std;

DEFINE_LOG_CATEGORY(LogMyGame);


#pragma region Debug Function
void SoundManager_Fmod::DebugLog(FString str) {
	FString errorString = str;
	GEngine->AddOnScreenDebugMessage(-1, 10.0, FColor::Blue, *errorString);
}

void SoundManager_Fmod::DebugLog(float f) {
	FString errorString = FString::SanitizeFloat(f);
	GEngine->AddOnScreenDebugMessage(-1, 10.0, FColor::Blue, *errorString);
}

bool SoundManager_Fmod::FMOD_ERROR_CHECK(FMOD_RESULT result) {
	if (result != FMOD_OK) {
		DebugLog(result);
		return false;
	}
	return true;
}
#pragma endregion

std::vector<float> cBuffer;
float *oBuffer = NULL;


FMOD_RESULT F_CALLBACK
DSPCallback(FMOD_DSP_STATE *dsp_state, float *inbuffer, float *outbuffer, unsigned int length, int inchannels, int *outchannels)
{
	FMOD::DSP *thisdsp = (FMOD::DSP *)dsp_state->instance;
	oBuffer = inbuffer;

	/* for every sample every channel */
	for (unsigned int samp = 0; samp < length; samp++) {
		for (signed int chan = 0; chan < *outchannels; chan++) {
			int n = (samp * inchannels) + chan;
			/* mute the channel after taking copying from it */
			//outbuffer[n] = inbuffer[n];	/* Comment it out if want to have record playback*/
			//outbuffer[n] = 0;	/* Comment it out if want to have record playback*/
		}
	}
	
	//FString errorString = FString::SanitizeFloat(666);
	//GEngine->AddOnScreenDebugMessage(-1, 10.0, FColor::Blue, *errorString);

	return FMOD_OK;
}

SoundManager_Fmod::SoundManager_Fmod() :_system(NULL), _channel(NULL), _sound(NULL)
{

}


SoundManager_Fmod::~SoundManager_Fmod()
{
	if (_sound)
	{
		_sound->release();
	}

	if (_system)
	{
		_system->close();
		_system->release();
	}
}



#pragma region Initialisation
int SoundManager_Fmod::Initialise()
{
	_channel = NULL;
	_dsp = NULL;
	_recorderFFT = NULL;

	FMOD_RESULT result = FMOD::System_Create(&_system);
	if (result != FMOD_OK)
	{
		return result;
	}
	else
	{
		result = _system->init(32, FMOD_INIT_NORMAL, 0);
		if (!FMOD_ERROR_CHECK(result))
			return false;
	}

	unsigned int version = 0;
	result = _system->getVersion(&version);
	if (!FMOD_ERROR_CHECK(result))
		return false;

	if (version < FMOD_VERSION) {
		DebugLog("FMOD Version is too old\n");
		//return false;
	}

	FMOD_DSP_DESCRIPTION dspdesc;
	memset(&dspdesc, 0, sizeof(dspdesc));

	strncpy_s(dspdesc.name, "My first DSP unit", sizeof(dspdesc.name));
	dspdesc.numinputbuffers = 1;
	dspdesc.numoutputbuffers = 1;
	dspdesc.read = DSPCallback;

	result = _system->createDSP(&dspdesc, &_myDSP);
	if (!FMOD_ERROR_CHECK(result))
		return false;

	result = _system->createDSPByType(FMOD_DSP_TYPE_FFT, &_dsp);
	_dsp->setParameterInt(FMOD_DSP_FFT_WINDOWSIZE, WINDOW_SIZE);
	_dsp->setParameterInt(FMOD_DSP_FFT_WINDOWTYPE, FMOD_DSP_FFT_WINDOW_HANNING);

	result = _system->createDSPByType(FMOD_DSP_TYPE_FFT, &_recorderFFT);
	_recorderFFT->setParameterInt(FMOD_DSP_FFT_WINDOWSIZE, WINDOW_SIZE);
	_recorderFFT->setParameterInt(FMOD_DSP_FFT_WINDOWTYPE, FMOD_DSP_FFT_WINDOW_HANNING);

	_yin = new YinPitchDetection();

	return 0;
}
#pragma endregion


#pragma region PlaySound
int SoundManager_Fmod::LoadSoundFromPath(std::string pathToFile)
{
	FMOD_RESULT result = _system->createSound(pathToFile.c_str(), FMOD_LOOP_NORMAL | FMOD_CREATESAMPLE, 0, &_sound);
	return result;
}

int SoundManager_Fmod::LoadSoundFromMemory(char* memoryPtr, unsigned int memorySize)
{
	FMOD_CREATESOUNDEXINFO sndinfo = { 0 };
	sndinfo.cbsize = sizeof(sndinfo);
	sndinfo.length = memorySize;

	FMOD_RESULT result = _system->createSound(memoryPtr, FMOD_OPENMEMORY | NULL | FMOD_CREATESAMPLE, &sndinfo, &_sound);
	return result;
}

void SoundManager_Fmod::PlaySound(float volume)
{
	_FFTHistory_linear.clear();
	_FFTHistory_log.clear();

	_system->playSound(_sound, 0, false, &_channel);
	_channel->setVolume(volume);
	_channel->getFrequency(&_samplingFrequency);
	_FFThistory_MaxSize = _samplingFrequency / WINDOW_SIZE;

	_channel->addDSP(0, _dsp);
	_dsp->setActive(true);
}

void SoundManager_Fmod::PauseSound(bool unPause)
{
	bool isPaused;
	_channel->getPaused(&isPaused);
	if (isPaused && unPause)
	{
		_channel->setPaused(false);
	}
	else if (!isPaused && !unPause)
	{
		_channel->setPaused(true);
	}
}

#pragma endregion




void SoundManager_Fmod::Update()
{
	_system->update();
}




#pragma region Record & Playback
int32 SoundManager_Fmod::InitialiseRecorder() {
	//Some local variables
	channel = NULL;
	recordedSound = NULL;
	unsigned int samplesRecorded = 0;
	unsigned int samplesPlayed = 0;
	bool dspEnabled = false;

	/*
	Determine latency in samples.
	*/
	int nativeChannels = 0;
	result = _system->getRecordDriverInfo(DEVICE_INDEX, NULL, 0, NULL, &nativeRate, NULL, &nativeChannels, NULL);
	if (!FMOD_ERROR_CHECK(result))
		return false;

	driftThreshold = (nativeRate * DRIFT_MS) / 1000;       /* The point where we start compensating for drift */
	desiredLatency = (nativeRate * LATENCY_MS) / 1000;     /* User specified latency */
	adjustedLatency = desiredLatency;                      /* User specified latency adjusted for driver update granularity */
	actualLatency = desiredLatency;                                 /* Latency measured once playback begins (smoothened for jitter) */

																		/*
																		Create user sound to record into, then start recording.
																		*/
	FMOD_CREATESOUNDEXINFO exinfo = { 0 };
	exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
	exinfo.numchannels = nativeChannels;
	exinfo.format = FMOD_SOUND_FORMAT_PCM16;
	exinfo.defaultfrequency = nativeRate;
	exinfo.length = nativeRate * sizeof(short) * nativeChannels; /* 1 second buffer, size here doesn't change latency */

	result = _system->createSound(0, FMOD_LOOP_NORMAL | FMOD_OPENUSER, &exinfo, &recordedSound);	//try stream?
	if (!FMOD_ERROR_CHECK(result))
		return false;

	result = _system->recordStart(DEVICE_INDEX, recordedSound, true);
	if (!FMOD_ERROR_CHECK(result))
		return false;

	//unsigned int soundLength = 0;
	result = recordedSound->getLength(&soundLength, FMOD_TIMEUNIT_PCM);
	if (!FMOD_ERROR_CHECK(result))
		return false;

	//result = _system->playSound(recordedSound, NULL, false, &channel);
	//if (!FMOD_ERROR_CHECK(result))
		//return false;

	return true;
}

int32 SoundManager_Fmod::InitialiseRecordBuffer() {


	return true;
}

int32 SoundManager_Fmod::StartRecordSound() {	
	//Determine how much has been recorded since we last checked
	recordPos = 0;
	result = _system->getRecordPosition(DEVICE_INDEX, &recordPos);
	if (result != FMOD_ERR_RECORD_DISCONNECTED) {
		if (!FMOD_ERROR_CHECK(result))
			return false;
	}

	lastRecordPos = 0;
	unsigned int recordDelta = (recordPos >= lastRecordPos) ? (recordPos - lastRecordPos) : (recordPos + soundLength - lastRecordPos);
	lastRecordPos = recordPos;
	samplesRecorded += recordDelta;

	//static unsigned int 
		minRecordDelta = (unsigned int)-1;

	if (recordDelta && (recordDelta < minRecordDelta))
	{
		minRecordDelta = recordDelta; /* Smallest driver granularity seen so far */
		adjustedLatency = (recordDelta <= desiredLatency) ? desiredLatency : recordDelta; /* Adjust our latency if driver granularity is high */
	}

	//Delay playback until our desired latency is reached.
	if (!channel && samplesRecorded >= adjustedLatency) {

		bool isPlaying = false;
		channel->isPlaying(&isPlaying);
		if (!isPlaying) {
			result = _system->playSound(recordedSound, 0, false, &channel);
			channel->addDSP(0, _myDSP);
			_myDSP->setActive(true);
			channel->addDSP(1, _recorderFFT);
			_recorderFFT->setActive(true);
			if (!FMOD_ERROR_CHECK(result))
				return false;
		}
	}




	if (channel)
	{
		/*
		Stop playback if recording stops.
		*/
		bool isRecording = false;
		result = _system->isRecording(DEVICE_INDEX, &isRecording);
		if (result != FMOD_ERR_RECORD_DISCONNECTED)
		{
			if (!FMOD_ERROR_CHECK(result))
				return false;
		}

		if (!isRecording)
		{
			result = channel->setPaused(true);
			if (!FMOD_ERROR_CHECK(result))
				return false;
		}

		/*
		Determine how much has been played since we last checked.
		*/
		playPos = 0;

		result = channel->getPosition(&playPos, FMOD_TIMEUNIT_PCM);
		if (!FMOD_ERROR_CHECK(result))
			return false;

		static unsigned int lastPlayPos = 0;
		unsigned int playDelta = (playPos >= lastPlayPos) ? (playPos - lastPlayPos) : (playPos + soundLength - lastPlayPos);
		lastPlayPos = playPos;
		samplesPlayed += playDelta;

		/*
		Compensate for any drift.
		*/
		int latency = samplesRecorded - samplesPlayed;
		actualLatency = (0.97f * actualLatency) + (0.03f * latency);

		int playbackRate = nativeRate;
		if (actualLatency < (int)(adjustedLatency - driftThreshold))
		{
			/* Play position is catching up to the record position, slow playback down by 2% */
			playbackRate = nativeRate - (nativeRate / 50);
		}
		else if (actualLatency >(int)(adjustedLatency + driftThreshold))
		{
			/* Play position is falling behind the record position, speed playback up by 2% */
			playbackRate = nativeRate + (nativeRate / 50);
		}

		channel->setFrequency((float)playbackRate);
		if (!FMOD_ERROR_CHECK(result))
			return false;

	}


	return true;
}


int32 SoundManager_Fmod::StopRecordSound() {

	return true;
}


int32 SoundManager_Fmod::PlayRecordedSound() {

	return true;
}

int32 SoundManager_Fmod::PauseRecordedSound() {


	return true;
}
#pragma endregion


float SoundManager_Fmod::GetPitch() {
	float pitch = 0.0f;
	if (oBuffer != NULL)
		pitch = _yin->YinPitchTracking(oBuffer);

	if (pitch != -1)
		//DebugLog(pitch);
		return pitch;
	else
		return 0;
}


#pragma region Display TimeDomain 
int32 SoundManager_Fmod::InitialiseTimeDomain(int maxBars) {
	return 2048;
}


void SoundManager_Fmod::GetTimeDomain(float* bars) {
	if (oBuffer == NULL) {
		DebugLog("FU");
		return;

	}

	//spectrum = oBuffer;
	for (int i = 0; i < 2048; i++) {
		bars[i] = oBuffer[i];
		//DebugLog(i);
	}
}
#pragma endregion



#pragma region  Specturm
int SoundManager_Fmod::InitialiseSpectrum_Linear(int maxBands)
{
	int barSamples = (WINDOW_SIZE / 2) / maxBands;

	//calculates num fft samples per bar
	_numSamplesPerBar_linear.clear();
	for (int i = 0; i < maxBands; ++i)
	{
		_numSamplesPerBar_linear.push_back(barSamples);
	}
	return _numSamplesPerBar_linear.size(); //effectiveBars
}

void SoundManager_Fmod::GetSpectrum_Linear(float* spectrum)
{
	FMOD_DSP_PARAMETER_FFT* dspFFT = NULL;
	FMOD_RESULT result = _dsp->getParameterData(FMOD_DSP_FFT_SPECTRUMDATA, (void **)&dspFFT, 0, 0, 0);
	if (dspFFT)
	{
		// Only read / display half of the buffer typically for analysis 
		// as the 2nd half is usually the same data reversed due to the nature of the way FFT works.
		int length = dspFFT->length / 2;
		int numChannels = dspFFT->numchannels;

		if (length > 0)
		{
			int indexFFT = 0;
			for (int index = 0; index < _numSamplesPerBar_linear.size(); ++index)	/*For each octave bar*/
			{
				for (int frec = 0; frec < _numSamplesPerBar_linear[index]; ++frec)
				{
					for (int channel = 0; channel < numChannels; ++channel)
					{
						spectrum[index] += dspFFT->spectrum[channel][indexFFT];
					}
					++indexFFT;
				}
				spectrum[index] /= (float)(_numSamplesPerBar_linear[index] * numChannels);
				
			}
		}
	}
}

int SoundManager_Fmod::InitialiseSpectrum_Log(int maxBars)
{
	//calculates octave frequency
	std::vector<int> frequencyOctaves;
	frequencyOctaves.push_back(0);
	for (int i = 1; i < 13; ++i)
	{
		frequencyOctaves.push_back((int)((44100 / 2) / (float)pow(2, 12 - i)));
	}

	int bandWidth = (44100 / WINDOW_SIZE);
	int bandsPerOctave = maxBars / 12; //octaves

									   //calculates num fft samples per bar
	_numSamplesPerBar_log.clear();
	for (int octave = 0; octave < 12; ++octave)
	{
		int indexLow = frequencyOctaves[octave] / bandWidth;
		int indexHigh = (frequencyOctaves[octave + 1]) / bandWidth;
		int octavaIndexes = (indexHigh - indexLow);

		//DebugLog(octavaIndexes);

		if (octavaIndexes > 0)
		{
			if (octavaIndexes <= bandsPerOctave)
			{
				for (int count = 0; count < octavaIndexes; ++count)
				{
					_numSamplesPerBar_log.push_back(1);
				}
			}
			else
			{
				for (int count = 0; count < bandsPerOctave; ++count)
				{
					_numSamplesPerBar_log.push_back(octavaIndexes / bandsPerOctave);
				}
			}
		}
		//DebugLog(_numSamplesPerBar_log.size());
	}

	return _numSamplesPerBar_log.size(); //effectiveBars
}

void SoundManager_Fmod::GetSpectrum_Log(float* spectrum)
{
	FMOD_DSP_PARAMETER_FFT* dspFFT = NULL;
	
	/* BGM */
	//FMOD_RESULT result = _dsp->getParameterData(FMOD_DSP_FFT_SPECTRUMDATA, (void **)&dspFFT, 0, 0, 0);

	/* Recorder */
	FMOD_RESULT result = _recorderFFT->getParameterData(FMOD_DSP_FFT_SPECTRUMDATA, (void **)&dspFFT, 0, 0, 0);

	//FString str2 = "";
	FString TestHUDString = FString(TEXT("This is my test FString."));
	
	if (dspFFT)
	{
		// Only read / display half of the buffer typically for analysis 
		// as the 2nd half is usually the same data reversed due to the nature of the way FFT works.
		int length = dspFFT->length / 2;
		int numChannels = dspFFT->numchannels;

		if (length > 0)
		{
			int indexFFT = 0;
			for (int index = 0; index < _numSamplesPerBar_log.size(); ++index)
			{
				for (int frec = 0; frec < _numSamplesPerBar_log[index]; ++frec)
				{
					for (int channel = 0; channel < numChannels; ++channel)
					{
						spectrum[index] += dspFFT->spectrum[channel][indexFFT];
					}
					++indexFFT;
				}
				spectrum[index] /= (float)(_numSamplesPerBar_log[index] * numChannels);
				//UE_LOG(LogMyGame, Log, TEXT("Spectrum:  %d ."), spectrum[index]);
				
			}
		}
	}
}
#pragma endregion


#pragma region Beat Detection

void SoundManager_Fmod::initializeBeatDetector() {
	int bandSize = _samplingFrequency / WINDOW_SIZE;

	_beatDetector_bandLimits.clear();
	_beatDetector_bandLimits.reserve(4); // bass + lowMidRange * 2

										 // BASS 60 hz - 130 hz (Kick Drum)
	_beatDetector_bandLimits.push_back(60 / bandSize);
	_beatDetector_bandLimits.push_back(130 / bandSize);

	// LOW MIDRANGE 301 hz - 750 hz (Snare Drum)
	_beatDetector_bandLimits.push_back(301 / bandSize);
	_beatDetector_bandLimits.push_back(750 / bandSize);

	_FFTHistory_beatDetector.clear();
}

void SoundManager_Fmod::getBeat(float* spectrum, float* averageSpectrum, bool& isBass, bool& isLowM, float& time) {
	FMOD_DSP_PARAMETER_FFT* dspFFT = NULL;
	FMOD_RESULT result = _dsp->getParameterData(FMOD_DSP_FFT_SPECTRUMDATA, (void **)&dspFFT, 0, 0, 0);

	if (dspFFT)
	{
		// Only read / display half of the buffer typically for analysis 
		// as the 2nd half is usually the same data reversed due to the nature of the way FFT works.
		int length = dspFFT->length / 2;
		int numChannels = dspFFT->numchannels;

		if (length > 0)
		{
			int numBands = _beatDetector_bandLimits.size() / 2;

			for (int numBand = 0; numBand < numBands; ++numBand)
			{
				for (int indexFFT = _beatDetector_bandLimits[numBand];
					indexFFT < _beatDetector_bandLimits[numBand + 1];
					++indexFFT)
				{
					for (int channel = 0; channel < numChannels; ++channel)
					{
						spectrum[numBand] += dspFFT->spectrum[channel][indexFFT];
					}
				}
				spectrum[numBand] /= (_beatDetector_bandLimits[numBand + 1] - _beatDetector_bandLimits[numBand]) * numChannels;
			}

			if (_FFTHistory_beatDetector.size() > 0)
			{
				fillAverageSpectrum(averageSpectrum, numBands, _FFTHistory_beatDetector);

				std::vector<float> varianceSpectrum;
				varianceSpectrum.resize(numBands);
				fillVarianceSpectrum(varianceSpectrum.data(), numBands, _FFTHistory_beatDetector, averageSpectrum);
				isBass = (spectrum[0] - 0.05) > beatThreshold(varianceSpectrum[0]) * averageSpectrum[0];
				isLowM = (spectrum[1] - 0.005) > beatThreshold(varianceSpectrum[1]) * averageSpectrum[1];

				if (isBass || isLowM) {
					unsigned int beatTime = 0;
					result = _channel->getPosition(&beatTime, FMOD_TIMEUNIT_MS);
					float ms = beatTime;
					//float second = ms / 1000;
					//DebugLog(second);
					time = ms / 1000;
				}
			}

			std::vector<float> fftResult;
			fftResult.reserve(numBands);
			for (int index = 0; index < numBands; ++index)
			{
				fftResult.push_back(spectrum[index]);
			}

			if (_FFTHistory_beatDetector.size() >= _FFThistory_MaxSize)
			{
				_FFTHistory_beatDetector.pop_front();
			}

			_FFTHistory_beatDetector.push_back(fftResult);
		}
	}
}

void SoundManager_Fmod::fillAverageSpectrum(float* averageSpectrum, int numBands, const FFTHistoryContainer& fftHistory) {
	for (FFTHistoryContainer::const_iterator fftResult_it = fftHistory.cbegin();
		fftResult_it != fftHistory.cend();
		++fftResult_it)
	{
		const std::vector<float>& fftResult = *fftResult_it;

		for (int index = 0; index < fftResult.size(); ++index)
		{
			averageSpectrum[index] += fftResult[index];
		}
	}

	for (int index = 0; index < numBands; ++index)
	{
		averageSpectrum[index] /= (fftHistory.size());
	}
}

void SoundManager_Fmod::fillVarianceSpectrum(float* varianceSpectrum, int numBands, const FFTHistoryContainer& fftHistory, const float* averageSpectrum) {
	for (FFTHistoryContainer::const_iterator fftResult_it = fftHistory.cbegin();
		fftResult_it != fftHistory.cend();
		++fftResult_it)
	{
		const std::vector<float>& fftResult = *fftResult_it;

		for (int index = 0; index < fftResult.size(); ++index)
		{
			varianceSpectrum[index] += (fftResult[index] - averageSpectrum[index]) * (fftResult[index] - averageSpectrum[index]);
		}
	}

	for (int index = 0; index < numBands; ++index)
	{
		varianceSpectrum[index] /= (fftHistory.size());
	}
}

float SoundManager_Fmod::beatThreshold(float variance) {
	//return -15 * variance + 1.55;
	return -15 * variance + 1.55;
}

#pragma endregion
