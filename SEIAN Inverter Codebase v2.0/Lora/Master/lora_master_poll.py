from SX127x.LoRa import LoRa
from SX127x.board_config import BOARD
import time
import re

BOARD.setup()

slave_map = {
    "01": 0x01,
    "02": 0x02,
    "03": 0x03,
    # ... up to 10
}

MAX_RETRIES = 3
RESPONSE_TIMEOUT = 1.5  # seconds

class LoRaMaster(LoRa):
    def __init__(self, slave_map, verbose=False):
        super(LoRaMaster, self).__init__(verbose)
        self.slave_map = slave_map
        self.set_mode(self.MODES['STDBY'])
        self.set_dio_mapping([0] * 6)
        self.buffer = []

    def set_sync_word(self, sync_word):
        self.write_register(0x39, sync_word)
        time.sleep(0.1)

    def send_packet(self, text):
        self.write_payload([ord(c) for c in text])
        self.set_mode(self.MODES['TX'])
        while self.get_mode() == self.MODES['TX']:
            time.sleep(0.01)
        self.set_mode(self.MODES['RXCONT'])

    def send_request(self):
        self.send_packet("@REQ\n")

    def send_ack(self, inv_id):
        ack_msg = f"@ACK:{inv_id}"
        self.send_packet(ack_msg)

    def check_receive(self):
        if self.get_irq_flags()['rx_done']:
            payload = self.read_payload(nocheck=True)
            self.clear_irq_flags(RxDone=1)
            msg = ''.join(chr(b) for b in payload).strip()
            self.buffer.append(msg)

    def receive_response(self, timeout=1.5):
        start = time.time()
        while time.time() - start < timeout:
            self.check_receive()
            if self.buffer:
                return self.buffer.pop(0)
            time.sleep(0.02)
        return None

    def parse_response(self, msg):
        m = re.match(r"@RESP:([0-9A-Fa-f]{2}):(.*)", msg)
        if not m:
            return None, None
        inv_id, data = m.group(1), m.group(2)
        return inv_id, data

    def poll_inverter(self, inv_id, sync_word):
        for attempt in range(1, MAX_RETRIES + 1):
            print(f"→ [{inv_id}] Try {attempt}/{MAX_RETRIES}")
            self.set_sync_word(sync_word)
            self.send_request()
            resp = self.receive_response(timeout=RESPONSE_TIMEOUT)
            if resp:
                rx_id, data = self.parse_response(resp)
                if rx_id == inv_id:
                    print(f"✔️ Received from {inv_id}: {data}")
                    self.send_ack(inv_id)
                    return data
                else:
                    print(f"⚠️ Mismatched ID: got {rx_id}, expected {inv_id}")
            else:
                print(f"⚠️ No response from {inv_id}")
        print(f"❌ Failed to reach inverter {inv_id}")
        return None

    def poll_all_inverters(self):
        results = {}
        for inv_id, sync in self.slave_map.items():
            print(f"\n--- Polling Inverter {inv_id} (sync=0x{sync:02X}) ---")
            data = self.poll_inverter(inv_id, sync)
            if data:
                results[inv_id] = data
        return results

    def run(self, cycles=0):
        self.set_mode(self.MODES['RXCONT'])
        count = 0
        try:
            while True:
                count += 1
                print(f"\n=== Polling Cycle {count} ===")
                data = self.poll_all_inverters()
                print("📦 Data:", data)
                if cycles and count >= cycles:
                    break
                time.sleep(3)
        finally:
            self.set_mode(self.MODES['SLEEP'])
            BOARD.teardown()

if __name__ == "__main__":
    master = LoRaMaster(slave_map)
    master.run(cycles=3)

