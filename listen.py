import errno
import socket
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

class ANSI:
    RESET  = "\033[0m"
    BLACK  = "\033[30m"
    RED    = "\033[31m"
    GREEN  = "\033[32m"
    YELLOW = "\033[33m"
    BLUE   = "\033[34m"
    MAGENTA= "\033[35m"
    CYAN   = "\033[36m"
    WHITE  = "\033[37m"

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

        self.client_queues = {} 
        self.client_threads = {} 

        self.dispatcher_thread = None

    def generate_filename_from_addr(self, addr: tuple[str, int]) -> str:
        ip, port = addr
        return f"{ip}-{port}-audio.bin"

    def bind(self) -> bool:
        self.sock.settimeout(1.0)
        try:
            self.sock.bind(self.connection_tuple)
            logger.info(f"successfully bound to interface {self.ip_address}:{self.port}")
            return True
        except PermissionError:
            logger.critical(
                f"failed to bind to interface: {self.ip_address}:{self.port} requires admin/sudo privileges!"
            )
            return False

    def client_writer(self, addr: tuple[str, int]) -> None:
            ip, port = addr
            filename = self.connected_clients[addr]
            
            total_bytes = 0
            packet_count = 0
            
            fh = open(filename, "ab")
            client_q = self.client_queues[addr]
            
            while not self.stop_event.is_set():
                try:
                    data = client_q.get(timeout=2.0)
                    

                    while True:
                        fh.write(data)
                        total_bytes += len(data)
                        packet_count += 1
                        
                        try:
                            data = client_q.get_nowait()
                        except Empty:
                            break
                    
                    fh.flush()

                except Empty:
                    if total_bytes > 0:
                        size_kb = total_bytes / 1024
                        logger.info(
                            f"{ip}:{port} "
                            f"sent {ANSI.GREEN}{size_kb:.2f} KB{ANSI.RESET} "
                            f"over {ANSI.YELLOW}{packet_count}{ANSI.RESET} packets"
                        )
                        total_bytes = 0
                        packet_count = 0
                    continue 
                except OSError:
                    break
            fh.close()

    def dispatcher(self) -> None:
        """
        Single dispatcher thread that routes ingress packets to per-client queues.
        This preserves per-client ordering without locks.
        """
        while not self.stop_event.is_set():
            try:
                data, addr = self.q.get(timeout=3.0)
            except Empty:
                continue  # nothing to process yet, keep waiting
            except OSError:
                break

            if addr not in self.client_queues:
                self.client_queues[addr] = Queue(maxsize=5_000)

                t = threading.Thread(target=self.client_writer, args=(addr,), daemon=True)
                self.client_threads[addr] = t
                t.start()

            ip, port = addr
            try:
                self.client_queues[addr].put_nowait(data)
            except Full:
                logger.warning("queue full; dropping packet from %s:%s", ip, port)

            self.q.task_done()

    def worker(self) -> None:
        """
        Kept for compatibility with your structure, but ordering + correctness is now handled
        by per-client writer threads via dispatcher().
        """
        # If you ever want a generic worker pool, you'd need per-client ordering logic here.
        while not self.stop_event.is_set():
            try:
                _ = self.q.get(timeout=.5)
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
                    logger.info(f"{ANSI.YELLOW}{ip}:{port}{ANSI.RESET} joined the server! Generated RAW audio file: {filename}")

                try:
                    self.q.put_nowait((data, addr))
                except Full:
                    logger.warning("queue full; dropping packet from %s:%s", ip, port)

            except socket.timeout:
                continue

            except OSError as e:
                if e.errno == errno.EMSGSIZE:
                    logger.critical(
                        "the packet size of the server needs to be adjusted to match the client (--packet-size)"
                    )
                else:
                    logger.exception(f"unexpected OS error occurred: {e}")
                self.stop()

            except KeyboardInterrupt:
                self.stop()

    def start_workers(self, num: int = 4):
        self.dispatcher_thread = threading.Thread(target=self.dispatcher, daemon=True)
        self.dispatcher_thread.start()

    def start(self) -> None:
        logger.info("starting Wiretap...")
        if not self.bind():
            return
        self.start_workers()
        self.listen()

    def stop(self) -> None:
        logger.info("stopping Wiretap...")
        self.stop_event.set()
        self.sock.close()

def parse_args():
    parser = argparse.ArgumentParser(
        description="UDP Wiretap Server (https://github.com/Drew-Alleman/wiretap)"
    )
    parser.add_argument(
        "--bind-ip",
        default="0.0.0.0",
        help="IP address the server should bind to (default: 0.0.0.0)"
    )
    parser.add_argument(
        "--port",
        type=int,
        default=53,
        help="UDP port to listen on (default: 53)"
    )
    parser.add_argument(
        "--packet-size",
        type=int,
        default=1024,
        help="Packet size the client is configured to (default: 1024)"
    )

    return parser.parse_args()

if __name__ == "__main__":
    args = parse_args()

    wiretap = Wiretap(
        ip_address=args.bind_ip,
        port=args.port,
        packet_size=args.packet_size
    )
    print(f"""                                  {ANSI.RED}.
     {ANSI.RED}.             {ANSI.YELLOW} .   .'.{ANSI.RESET}     {ANSI.RED}\\   /{ANSI.RESET}
   {ANSI.RED}\\   /      {ANSI.YELLOW}.'. .' '.'   '{ANSI.RESET}  {ANSI.RED}-=  {ANSI.RESET}o{ANSI.RED}  =-{ANSI.RESET}
 {ANSI.RED}-=  {ANSI.RESET}o{ANSI.RED}  =-  {ANSI.YELLOW}.'   '{ANSI.RESET}              {ANSI.RED}/{ANSI.RESET} | {ANSI.RED}\\{ANSI.RESET}
   {ANSI.RED}/{ANSI.RESET} | {ANSI.RED}\\{ANSI.RESET}                          |
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
     """)
    wiretap.start()