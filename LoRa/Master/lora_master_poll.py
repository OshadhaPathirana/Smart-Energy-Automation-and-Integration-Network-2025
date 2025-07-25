from SX127x.LoRa import LoRa
from SX127x.board_config import BOARD
import time
import re

# Map inverter IDs to sync words
slave_map = {
    "01": 0x01,
    "02": 0x02,
    "03": 0x03,
    # … up to 10
}

class LoRaMaster(LoRa):
    def __init__(self, slave_map, verbose=False):
        super(LoRaMaster, self).__init__(verbose)
        self.slave_map = slave_map
        self.set_mode(self.MODES['STDBY'])
        self.set_dio_mapping([0] * 6)
        self.buffer = []

    def set_sync_word(self, sync_word):
        self.write_register(0x39, sync_word)  # RegSyncWord
        val = self.read_register(0x39)
        if val != sync_word:
            raise RuntimeError(f"Sync word verify failed: wrote {sync_word:#02x}, read {val:#02x}")
        time.sleep(0.1)

    def send_request(self):
        msg = "@REQ\n"
        self.write_payload([ord(c) for c in msg])
        self.set_mode(self.MODES['TX'])
        while self.get_mode() == self.MODES['TX']:
            time.sleep(0.01)
        self.set_mode(self.MODES['RXCONT'])

    def check_receive(self):
        if self.get_irq_flags()['rx_done']:
            payload = self.read_payload(nocheck=True)
            self.clear_irq_flags(RxDone=1)
            msg = ''.join(chr(b) for b in payload).strip()
            self.buffer.append(msg)

    def receive_response(self, timeout=1.0):
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

    def poll_once(self):
        results = {}
        for inv_id, sync in self.slave_map.items():
            print(f"\n--- Polling Inverter {inv_id} (sync=0x{sync:02X}) ---")
            self.set_sync_word(sync)
            self.send_request()
            resp = self.receive_response(timeout=1.5)
            if resp:
                inv, data = self.parse_response(resp)
                if inv == inv_id:
                    results[inv] = data
                    print(f"✔️ Got data from {inv}: {data}")
                else:
                    print(f"⚠️ Invalid response ID '{inv}', expected '{inv_id}'")
            else:
                print(f"⚠️ No response from {inv_id}")
            time.sleep(0.2)
        return results

    def run(self, cycles=0):
        self.set_mode(self.MODES['RXCONT'])
        count = 0
        try:
            while True:
                count += 1
                print(f"\n=== Query Cycle {count} ===")
                data = self.poll_once()
                print("Results:", data)
                if cycles and count >= cycles:
                    break
                time.sleep(3)
        finally:
            self.set_mode(self.MODES['SLEEP'])
            BOARD.teardown()

if __name__ == "__main__":
    BOARD.setup()
    master = LoRaMaster(slave_map, verbose=False)
    master.run(cycles=3)  # Poll 3 cycles then stop (0 for infinite)
