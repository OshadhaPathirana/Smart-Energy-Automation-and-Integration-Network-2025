from SX127x.LoRa import *
from SX127x.board_config import BOARD
import time

BOARD.setup()

class LoRaMaster(LoRa):
    def __init__(self, verbose=False):
        super(LoRaMaster, self).__init__(verbose)
        self.set_mode(MODE.STDBY)
        self.set_dio_mapping([0] * 6)
        self.slave_ids = ["01", "02", "03"]  # Add your slave IDs here

    def send_request(self, slave_id):
        msg = f"@REQ:{slave_id}\n"
        print(f"Sending: {msg.strip()}")
        self.write_payload([ord(c) for c in msg])
        self.set_mode(MODE.TX)
        while self.get_mode() == MODE.TX:
            time.sleep(0.1)
        self.set_mode(MODE.RXCONT)
    
    def receive_response(self, timeout=2.0):
        start_time = time.time()
        buffer = ""
        while time.time() - start_time < timeout:
            if self.get_irq_flags()['rx_done']:
                payload = self.read_payload(nocheck=True)
                self.clear_irq_flags(RxDone=1)
                msg = ''.join([chr(b) for b in payload])
                print(f"Received: {msg.strip()}")
                return msg.strip()
            time.sleep(0.1)
        print("Timeout waiting for response.")
        return None

    def poll_slaves(self):
        self.set_mode(MODE.RXCONT)
        while True:
            for slave_id in self.slave_ids:
                self.send_request(slave_id)
                response = self.receive_response()
                if response:
                    # Optionally parse and store response
                    pass
                time.sleep(1)  # Delay before next request

if __name__ == "__main__":
    lora = LoRaMaster(verbose=False)
    try:
        lora.poll_slaves()
    except KeyboardInterrupt:
        print("Stopping master.")
    finally:
        lora.set_mode(MODE.SLEEP)
        BOARD.teardown()
