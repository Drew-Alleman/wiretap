import socket
import wave
import sys

UDP_IP = "0.0.0.0" # Bind to all interfaces to be safe
UDP_PORT = 443
OUTPUT_FILENAME = "sound.wav"
CHANNELS = 2
SAMPLE_RATE = 48000
SAMPLE_WIDTH = 2

def start_listener():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    # Set a timeout so the loop doesn't block CTRL+C forever
    sock.settimeout(1.0) 
    
    try:
        sock.bind((UDP_IP, UDP_PORT))
    except PermissionError:
        print("[-] Error: Port 443 requires Admin/Sudo privileges!")
        return

    print(f"[*] Siphon Listener Active on {UDP_PORT}...")
    
    # Open the WAV file outside the loop
    wav_file = wave.open(OUTPUT_FILENAME, 'wb')
    wav_file.setnchannels(CHANNELS)
    wav_file.setsampwidth(SAMPLE_WIDTH)
    wav_file.setframerate(SAMPLE_RATE)

    try:
        while True:
            try:
                data, addr = sock.recvfrom(65535)
                wav_file.writeframes(data)
                print(f"[+] Captured {len(data)} bytes from {addr[0]}", end='\r')
            except socket.timeout:
                # This lets the loop check for CTRL+C every 1 second
                continue 
    except KeyboardInterrupt:
        print(f"\n[!] Interrupt received. Finalizing WAV file...")
    finally:
        # CRITICAL: This ensures the WAV header is written even if you crash
        wav_file.close()
        sock.close()
        print(f"[*] File Saved: {OUTPUT_FILENAME}")

if __name__ == "__main__":
    start_listener()