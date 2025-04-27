import os, time
from influxdb_client_3 import InfluxDBClient3, Point
from influxdb_client import InfluxDBClient
from influxdb_client.client.write_api import SYNCHRONOUS

# InfluxDB Configuration
INFLUXDB_TOKEN = "kBLLkVYJr_s0oz0bqfnZDFk8AH-Kp8P56uwqWeZWfgJs7WV1HfHknQpyDMiXJbsqaReU_AOxtdq2MB6_sFol7w=="
token = INFLUXDB_TOKEN
org = "Dyson Sphere"
host = "https://us-east-1-1.aws.cloud2.influxdata.com"
database = "Bucket1"

client = InfluxDBClient3(host=host, token=token, org=org)
query_client = InfluxDBClient(url=host, token=token, org=org)

pot = 0

class PIDController:
    def __init__(self, Kp, Ki, Kd, setpoint):
        self.Kp = Kp
        self.Ki = Ki
        self.Kd = Kd
        self.setpoint = setpoint
        self.previous_error = 0
        self.integral = 0

    def compute(self, measured_value):
        error = self.setpoint - measured_value
        P = self.Kp * error
        self.integral += error
        I = self.Ki * self.integral
        derivative = error - self.previous_error
        D = self.Kd * derivative
        self.previous_error = error
        return P + I + D

def get_latest_voltage():
    query = f"""
    from(bucket: "{database}")
      |> range(start: -6h)
      |> filter(fn: (r) => r["_measurement"] == "Inverters")
      |> filter(fn: (r) => r["_field"] == "Voltage")
      |> last()
    """
    influx_data = []
    try:
        tables = query_client.query_api().query(query)
        for table in tables:
            for record in table.records:
                # print(f"printing record: {record}")
                if (record["Inverter_ID"] == '5'):
                    global pot
                    pot = record.get_value()
                else:
                    influx_data.append(record.get_value() / 1000)
        if influx_data:
            return influx_data[0]
    except Exception as e:
        print(f"Error querying data: {e}")
    return 230  # Fallback to safe default voltage

def write_voltage_to_influx(predicted_voltage):
    predicted_data = {
        "point1": {"Inverter_ID": "1", "Measurement": "voltage", "Value": predicted_voltage[1]},
        "point2": {"Inverter_ID": "2", "Measurement": "voltage", "Value": predicted_voltage[0]},
        "point3": {"Inverter_ID": "3", "Measurement": "voltage", "Value": predicted_voltage[0]},
        "point4": {"Inverter_ID": "4", "Measurement": "voltage", "Value": predicted_voltage[0]}
    }
    for key in predicted_data:
        point = (
            Point("ML")
            .tag("Inverter_ID", predicted_data[key]["Inverter_ID"])
            .field(predicted_data[key]["Measurement"], predicted_data[key]["Value"] * 1000)
        )
        client.write(database=database, record=point)
    print("Data Written to InfluxDB.")

def simulate_inverter(initial_voltage, target_voltage, Kp, Ki, Kd):
    pid = PIDController(Kp, Ki, Kd, target_voltage)
    current_voltage_1 = initial_voltage
    current_voltage_2 = 230
    time_step = 0.01
    time_elapsed = 0
    predicted_voltage = [0,50]

    while abs(target_voltage - current_voltage_1) > 1 or abs(target_voltage - current_voltage_2) > 1:
        control_signal_1 = pid.compute(current_voltage_1)
        control_signal_2 = pid.compute(current_voltage_2)

        current_voltage_1 += control_signal_1 * time_step
        current_voltage_2 += control_signal_2 * time_step

        predicted_voltage[0] = int((current_voltage_1 + current_voltage_2) / 2)
        write_voltage_to_influx(predicted_voltage)

        print(f"{time_elapsed:.2f}\t\t{current_voltage_1:.2f}\t\t{control_signal_1:.2f}")
        time.sleep(time_step)
        time_elapsed += time_step

    return target_voltage

def simulate_inverter_increasing(initial_voltage, target_voltage, Kp, Ki, Kd):
    pid = PIDController(Kp, Ki, Kd, target_voltage)
    current_voltage = initial_voltage
    time_step = 0.1
    time_elapsed = 0
    predicted_voltage = [0,0]

    while abs(target_voltage - current_voltage) > 0.1:
        control_signal = pid.compute(current_voltage)
        current_voltage += control_signal * time_step

        predicted_voltage[0] = int(current_voltage)
        predicted_voltage[1] = int(current_voltage)
        write_voltage_to_influx(predicted_voltage)

        print(f"{time_elapsed:.2f}\t\t{current_voltage:.2f}\t\t{control_signal:.2f}")
        time.sleep(time_step)
        time_elapsed += time_step

# === Continuous Execution ===
write_voltage_to_influx([230,230])

while (True):
    initial_voltage = get_latest_voltage()
    print(f"\nInitial voltage: {initial_voltage} V")

    if (pot ==1):
        
    # Step 1: Lower voltage to intermediate target
        target_voltage = 50
        print(f"Target voltage: {target_voltage} V")
        val = simulate_inverter(initial_voltage, target_voltage, Kp=125, Ki=1, Kd=2.)

    # Step 2: Increase voltage back to 230V
        simulate_inverter_increasing(val, 230, Kp=2, Ki=0.25, Kd=7)

        print("One full cycle completed. Waiting before next iteration...\n")
        time.sleep(0.1)  # Pause before next cycle
        pot = 0

