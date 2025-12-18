import socket
import wave
import logging
import argparse
import threading
from queue import Queue, Full, Empty

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s | %(levelname)s | %(message)s",
    datefmt="%Y-%m-%d %I:%M:%S %p"
)

logger = logging.getLogger(__name__)

CHANNELS = 2
SAMPLE_RATE = 48000
SAMPLE_WIDTH = 2

class Wiretap:
    def __init__(self, ip_address: str, port: int, packet_size: int) -> None:
        self.ip_address = ip_address
        self.port = port
        self.connection_tuple = (self.ip_address, self.port)
        self.connected_clients = {}

        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.packet_size = packet_size

        self.q = Queue(maxsize=10_000)
        self.stop_event = threading.Event()

        self.client_queues = {}   # {(ip, port): Queue}
        self.client_threads = {}  # {(ip, port): Thread}

        self.dispatcher_thread = None

    # NAT translation safe filename (avoid collisions for multiple clients on same IP)
    def generate_filename_from_addr(self, addr: tuple[str, int]) -> str:
        ip, port = addr
        return f"{ip}-{port}-sound.wav"

    def bind(self) -> bool:
        self.sock.settimeout(1.0)
        try:
            self.sock.bind(self.connection_tuple)
            logger.info(f"Successfully bound to interface {self.ip_address}:{self.port}")
            return True
        except PermissionError:
            logger.critical(
                f"Failed to bind to interface: {self.ip_address}:{self.port} requires Admin/Sudo privileges!"
            )
            return False

    def client_writer(self, addr: tuple[str, int]) -> None:
        """
        One writer thread per client => packets are written in order WITHOUT locks.
        """
        ip, port = addr
        filename = self.connected_clients[addr]

        wav_file = wave.open(filename, 'wb')
        wav_file.setnchannels(CHANNELS)
        wav_file.setsampwidth(SAMPLE_WIDTH)
        wav_file.setframerate(SAMPLE_RATE)

        client_q = self.client_queues[addr]
        while not self.stop_event.is_set():
            try:
                data = client_q.get(timeout=0.5)
            except Empty:
                continue  # nothing to process yet, keep waiting
            except OSError:
                break

            wav_file.writeframes(data)

            logger.info(f"Processing {len(data)} bytes from {ip}:{port}")

            client_q.task_done()

        wav_file.close()


    def dispatcher(self) -> None:
        """
        Single dispatcher thread that routes ingress packets to per-client queues.
        This preserves per-client ordering without locks.
        """
        while not self.stop_event.is_set():
            try:
                data, addr = self.q.get(timeout=0.5)
            except Empty:
                continue  # nothing to process yet, keep waiting
            except OSError:
                break

            # Ensure a per-client queue + writer exists
            if addr not in self.client_queues:
                self.client_queues[addr] = Queue(maxsize=5_000)

                t = threading.Thread(target=self.client_writer, args=(addr,), daemon=True)
                self.client_threads[addr] = t
                t.start()

            # Enqueue to that client's ordered queue
            ip, port = addr
            try:
                self.client_queues[addr].put_nowait(data)
            except Full:
                logger.warning("Queue full; dropping packet from %s:%s", ip, port)

            self.q.task_done()

    def worker(self) -> None:
        """
        Kept for compatibility with your structure, but ordering + correctness is now handled
        by per-client writer threads via dispatcher().
        """
        # If you ever want a generic worker pool, you'd need per-client ordering logic here.
        while not self.stop_event.is_set():
            try:
                _ = self.q.get(timeout=0.5)
                self.q.task_done()
            except Empty:
                continue  # nothing to process yet, keep waiting
            except OSError:
                break

    def listen(self) -> None:
        while not self.stop_event.is_set():
            try:
                data, addr = self.sock.recvfrom(self.packet_size)  # addr = (ip, port)
                ip, port = addr

                # NAT translation: identify clients by (ip, port)
                if addr not in self.connected_clients:
                    # keep your message format, but generate unique file per NAT'd client
                    filename = self.generate_filename_from_addr(addr)
                    self.connected_clients[addr] = filename
                    logger.info(f"New client: {ip}:{port} connected! Generated WAV file: {filename}")

                try:
                    self.q.put_nowait((data, addr))
                except Full:
                    logger.warning("Queue full; dropping packet from %s:%s", ip, port)

            except socket.timeout:
                continue
            except KeyboardInterrupt:
                self.stop()

    def start_workers(self, num: int = 4):
        self.dispatcher_thread = threading.Thread(target=self.dispatcher, daemon=True)
        self.dispatcher_thread.start()

    def start(self) -> None:
        logger.info("Starting Wiretap...")
        if not self.bind():
            return
        self.start_workers()
        self.listen()

    def stop(self) -> None:
        logger.info("Stopping Wiretap...")
        self.stop_event.set()
        self.sock.close()

def parse_args():
    parser = argparse.ArgumentParser(
        description="Wiretap UDP audio listener"
    )
    parser.add_argument(
        "--bind-ip",
        default="127.0.0.1",
        help="IP address to bind to (default: 127.0.0.1)"
    )
    parser.add_argument(
        "--port",
        type=int,
        default=443,
        help="UDP port to listen on (default: 443)"
    )
    parser.add_argument(
        "--packet-size",
        type=int,
        default=1024,
        help="Max UDP packet size in bytes (default: 1024)"
    )

    return parser.parse_args()

if __name__ == "__main__":
    args = parse_args()

    wiretap = Wiretap(
        ip_address=args.bind_ip,
        port=args.port,
        packet_size=args.packet_size
    )
    print("""                                  .
     .              .   .'.     \\   /
   \\   /      .'. .' '.'   '  -=  o  =-
 -=  o  =-  .'   '              / | \\
   / | \\                          |
     |                            |
     |                            |
     |                      .=====|
     |=====.                |.---.|
     |.---.|                ||=o=||
     ||=o=||                ||   ||
     ||   ||                ||   ||
     ||   ||                ||___||
     ||___||                |[:::]|
jgs  |[:::]|                '-----'
     '-----'
     
   [https://github.com/Drew-Alleman/wiretap]

""")
    wiretap.start()
