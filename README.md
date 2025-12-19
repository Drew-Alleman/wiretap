# wiretap
Stealthy Windows audio-capture tool with UDP exfiltration and randomized transmission intervals for Evasion-focused Red Team operations. (WIP)
<img width="1126" height="537" alt="console" src="https://github.com/user-attachments/assets/40f1fdae-5cd2-4c78-80d9-148e768dee51" />

<img width="1287" height="357" alt="wireshark" src="https://github.com/user-attachments/assets/85421209-d485-4f91-a986-82b1bcf4e2e0" />

# Importing the Audio File to Audacity
The generated files cannot be opened in standard audio applications because they contain raw audio data. A player or editor that supports RAW audio files is required; Audacity will be used to open and review the files.
<img width="1162" height="560" alt="image" src="https://github.com/user-attachments/assets/7e754a37-b4da-40f9-974f-6db6a254efa7" />
<img width="690" height="432" alt="image" src="https://github.com/user-attachments/assets/d5599ba3-b539-4b7f-bcf0-58449c59b7a9" />
<img width="998" height="600" alt="image" src="https://github.com/user-attachments/assets/1f35213e-f9b8-4397-a3dc-3db23583b7a0" />


# To-Do
## In-General
- Tutorial on how to import the raw audio into Audacity

## Python Listener
The following should be ready to go, I just need to test the code:
  - Allow multiple connections at once

## Wiretap
- Verbose setting
- Logging
- Test on different windows machines
- Option to set the "power" level (min threshold to pick up microphone)
- Better Sleep settings
- Better error handeling
- help menu option
- Optimize function to gather bytes from Microphone
- Argument to change packet size
- Enumeration packet to gather information about the device
- Option to only capture audio between certain time periods

### Administrator Settings (IDK yet)
- Persistance??
- Add option to remove the "wiretap.exe is using your microphone" on the windows task bar (needs admin)

