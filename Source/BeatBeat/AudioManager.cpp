#include "AudioManager.h"
#include "Paths.h"
#include "FileHelper.h"

UAudioManager::UAudioManager()
{
}

UAudioManager::~UAudioManager()
{

}

int32 UAudioManager::InitialiseManager()
{
	/*ptr to system manager*/
	_soundManager = std::unique_ptr<SoundManager_Fmod>(new SoundManager_Fmod());
	int m_result = _soundManager->Initialise();
	if (m_result > 0)
	{
		return false;
	}

	return 0;
}

int32 UAudioManager::InitialiseRecorder() {
	return _soundManager->InitialiseRecorder();
}

int32 UAudioManager::StartRecording() {
	return _soundManager->StartRecordSound();
}

int32 UAudioManager::StopRecording() {
	return _soundManager->StopRecordSound();
}

int32 UAudioManager::PlayRecordedSound() {
	return _soundManager->PlayRecordedSound();
}

int32 UAudioManager::PauseRecordedSound() {
	return _soundManager->PlayRecordedSound();
}

void UAudioManager::Update()
{
	_soundManager->Update();
}

int32 UAudioManager::InitSpectrum_Linear(const int32 maxBars)
{
	return _soundManager->InitialiseSpectrum_Linear(maxBars);
}

void UAudioManager::GetSpectrum_Linear(TArray<float>& frequencyValues, int32 numBars)
{
	frequencyValues.Init(0.0, numBars);
	_soundManager->GetSpectrum_Linear(frequencyValues.GetData());
}

int32 UAudioManager::InitSpectrum_Log(const int32 maxBars)
{
	return _soundManager->InitialiseSpectrum_Log(maxBars);
}

void UAudioManager::GetSpectrum_Log(TArray<float>& frequencyValues, int32 numBars)
{
	frequencyValues.Init(0.0, numBars);
	_soundManager->GetSpectrum_Log(frequencyValues.GetData());
}

int32 UAudioManager::InitTimeDomain(const int32 maxBars) {
	return _soundManager->InitialiseTimeDomain(maxBars);
}

void UAudioManager::GetTimeDomain(TArray<float>& frequencyValues, int32 numBars) {
	//frequencyValues.Init(0.0, numBars);
	frequencyValues.Init(0.0, 2048);
	_soundManager->GetTimeDomain(frequencyValues.GetData());
}

void UAudioManager::GetPitch(float &pitch) {
	pitch = _soundManager->GetPitch();
}

void UAudioManager::SortArray(UPARAM(ref) TArray<float> &inArray){
	//inArray.Sort();
	inArray.StableSort();

}

void UAudioManager::InitBeatDetector() {
	return _soundManager->initializeBeatDetector();
}

void UAudioManager::GetBeat(TArray<float>& frequencyValues, TArray<float>& frequencyAverageValues, bool& isBass, bool& isLowM, float& time) {
	frequencyValues.Init(0.0, 2);
	frequencyAverageValues.Init(0.0, 2);
	_soundManager->getBeat(frequencyValues.GetData(), frequencyAverageValues.GetData(), isBass, isLowM, time);
}

int32 UAudioManager::PlaySong(int numSong)
{
	FString path = FPaths::ProjectContentDir();
	FString songFile(path + "bgm.wav");

	uint8* data;
	unsigned int dataLength = 0;

	TArray <uint8> rawFile;
	FFileHelper::LoadFileToArray(rawFile, *songFile);

	data = rawFile.GetData();
	dataLength = rawFile.Num() * sizeof(uint8);

	int32 result = _soundManager->LoadSoundFromMemory(reinterpret_cast<char*>(data), dataLength);
	if (result > 0) {
		//for (int i = 0; i < 30; i++)
		//GEngine->AddOnScreenDebugMessage(-1, 10.0, FColor::Red, *songFile);

		return result; //missing file
	}

	_soundManager->PlaySound(1);


	return 0;
}

void UAudioManager::PauseSong(bool unPause)
{
	_soundManager->PauseSound(unPause);
}

int32 UAudioManager::PlaySongByName(FString name, float volume){
	FString path = FPaths::ProjectContentDir();
	//FString songFile(path + "bgm.wav");
	FString songFile(path + name);
	uint8* data;
	unsigned int dataLength = 0;

	TArray <uint8> rawFile;
	FFileHelper::LoadFileToArray(rawFile, *songFile);

	data = rawFile.GetData();
	dataLength = rawFile.Num() * sizeof(uint8);

	int32 result = _soundManager->LoadSoundFromMemory(reinterpret_cast<char*>(data), dataLength);
	if (result > 0) {
		//for (int i = 0; i < 30; i++)
		//GEngine->AddOnScreenDebugMessage(-1, 10.0, FColor::Red, *songFile);
		return result; //missing file
	}

	_soundManager->PlaySound(volume);


	return 0;
}