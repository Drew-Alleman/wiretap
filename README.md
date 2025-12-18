# wiretap
Stealthy Windows audio-capture tool with UDP exfiltration and randomized transmission intervals for Evasion-focused Red Team operations. (WIP)
<img width="1056" height="494" alt="image" src="https://github.com/user-attachments/assets/1bd6ae46-7dda-4eae-b3e7-205021d3c35e" />

<img width="1287" height="357" alt="image" src="https://github.com/user-attachments/assets/85421209-d485-4f91-a986-82b1bcf4e2e0" />

# To-Do

## In-General
- Tutorial on how to import the raw audio into Audacity

## Python Listener
The following should be ready to go, I just need to test the code:
  - Allow multiple connections at once
  - Argparse
  - Refactor

## Wiretap
- Verbose setting
- Logging
- Test on different windows machines
- Option to allow the user to toggle between ruunning in the Background or foreground.
- Option to set the "power" level (min threshold to pick up microphone)
- Better Sleep settings
- Better error handeling
- help menu option
- Optimize function to gather bytes from Microphone
- List all microphones on a device
- Add an option to change what microphone to use
- Argument to change packet size
- Enumeration packet to gather information about the device
- Option to only capture audio between certain time periods

### Administrator Settings (IDK yet)
- Persistance??
- Add option to remove the "wiretap.exe is using your microphone" on the windows task bar (needs admin)

