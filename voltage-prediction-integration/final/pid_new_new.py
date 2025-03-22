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

query = f"""
from(bucket: "{database}")
  |> range(start: -1h)
  |> filter(fn: (r) => r["_measurement"] == "Inverters")
  |> filter(fn: (r) => r["_field"] == "Voltage")
  |> last()
"""

influx_data = []
predicted_voltage = []
try:
    tables = query_client.query_api().query(query)
    print(tables)
    for table in tables:
        for record in table.records:
            print(f"reading --> Time: {record.get_time()}, Voltage: {record.get_value()}")
            influx_data.append(record.get_value()/1000)
            
            
            
except Exception as e:
    print(f"Error querying data: {e}")



class PIDController:
    def __init__(self, Kp, Ki, Kd, setpoint):
        self.Kp = Kp  # Proportional gain
        self.Ki = Ki  # Integral gain
        self.Kd = Kd  # Derivative gain
        self.setpoint = setpoint  # Desired voltage (230V)
        self.previous_error = 0
        self.integral = 0

    def compute(self, measured_value):
        # Calculate error
        error = self.setpoint - measured_value

        # Proportional term
        P = self.Kp * error

        # Integral term
        self.integral += error
        I = self.Ki * self.integral

        # Derivative term
        derivative = error - self.previous_error
        D = self.Kd * derivative

        # Save error for next iteration
        self.previous_error = error

        # Compute PID output
        output = P + I + D
        return output

def simulate_inverter(initial_voltage, target_voltage, Kp, Ki, Kd):
    
    
    


    # Initialize PID controller
    pid = PIDController(Kp, Ki, Kd, target_voltage)

    # Simulate the system
    current_voltage_1 = initial_voltage
    current_voltage_2 = 230
    time_step = 0.1  # Time step for simulation (in seconds)
    time_elapsed = 0

    # print(f"Starting voltage: {current_voltage_1}V, Target voltage: {target_voltage}V")
    # print("Time (s)\tVoltage (V)\tControl Signal")

    while (abs(target_voltage - current_voltage_1) > 1 or abs(target_voltage - current_voltage_2) > 1):  # Stop when close to target
        
        
        predicted_voltage = [0,0]
        
        # Compute control signal using PID
        control_signal_1 = pid.compute(current_voltage_1)
        control_signal_2 = pid.compute(current_voltage_2)

        # Simulate the effect of the control signal on the voltage
        # (This is a simple model; replace with your actual system dynamics)
        current_voltage_1 += control_signal_1 * time_step
        current_voltage_2 += control_signal_2 * time_step
        
        predicted_voltage[0] = int(current_voltage_1) 
        predicted_voltage[1] = int(current_voltage_2)
        
        predicted_data = {
            "point1": {
            "Inverter_ID": "1",
            "Measurement": "voltage",
            "Value": predicted_voltage[0],
            },
            "point2": {
            "Inverter_ID": "2",
            "Measurement": "voltage",
            "Value": predicted_voltage[1],
            },
            "point3": {
            "Inverter_ID": "3",
            "Measurement": "voltage",
            "Value": predicted_voltage[1],
            },
            "point4": {
            "Inverter_ID": "4",
            "Measurement": "voltage",
            "Value": predicted_voltage[1],
            }
        }
        
        for key in predicted_data:
            point = (
                Point("ML")
                .tag("Inverter_ID", predicted_data[key]["Inverter_ID"])
                .field(predicted_data[key]["Measurement"], predicted_data[key]["Value"]*1000)
                )
            client.write(database=database, record=point)
            time.sleep(0.1)

        print("Data Written to InfluxDB.")

        # Print results
        print(f"{time_elapsed:.2f}\t\t{current_voltage_1:.2f}\t\t{control_signal_1:.2f}")

        # Wait for the next time step
        time.sleep(time_step)
        time_elapsed += time_step
        
    
    current_voltage_3 = target_voltage
    return current_voltage_3  
    # while (current_voltage_3 < 230):  # Incrementally increase the voltage to 230
        
    #     predicted_voltage = [0]
        
    #     # Compute control signal using PID
    #     control_signal_3 = pid.compute(current_voltage_3)
        
    #     # Simulate the effect of the control signal on the voltage
    #     # (This is a simple model; replace with your actual system dynamics)
    #     current_voltage_3 += control_signal_3 * time_step
    #     if current_voltage_3 > 230:  # Ensure it doesn't exceed 230
    #         current_voltage_3 = 230
                
    #     predicted_voltage[0] = int(current_voltage_3)
        
    #     predicted_data = {
    #         "point1": {
    #         "Inverter_ID": "1",
    #         "Measurement": "voltage",
    #         "Value": predicted_voltage[0],
    #         },
    #         "point2": {
    #         "Inverter_ID": "2",
    #         "Measurement": "voltage",
    #         "Value": predicted_voltage[0],
    #         },
    #         "point3": {
    #         "Inverter_ID": "3",
    #         "Measurement": "voltage",
    #         "Value": predicted_voltage[0],
    #         },
    #         "point4": {
    #         "Inverter_ID": "4",
    #         "Measurement": "voltage",
    #         "Value": predicted_voltage[0],
    #         }
    #     }
    #     for key in predicted_data:
    #         point = (
    #             Point("ML")
    #             .tag("Inverter_ID", predicted_data[key]["Inverter_ID"])
    #             .field(predicted_data[key]["Measurement"], predicted_data[key]["Value"]*1000)
    #             )
    #         client.write(database=database, record=point)
    #         time.sleep(0.5)

    #     print("Data Written to InfluxDB.")

    #     # Print results
    #     print(f"{time_elapsed:.2f}\t\t{current_voltage_1:.2f}\t\t{control_signal_1:.2f}")

    #     # Wait for the next time step
    #     time.sleep(time_step)
    #     time_elapsed += time_step



def simulate_inverter_increasing(initial_voltage, target_voltage, Kp, Ki, Kd):
    pid = PIDController(Kp, Ki, Kd, target_voltage)
    current_voltage = initial_voltage
    time_step = 0.1  # Time step for simulation (in seconds)
    time_elapsed = 0
    
    while abs(target_voltage - current_voltage) > 0.1:  # Stop when close to target
        
        
        predicted_voltage = [0]
        
        # Compute control signal using PID
        control_signal = pid.compute(current_voltage)

        # Simulate the effect of the control signal on the voltage
        # (This is a simple model; replace with your actual system dynamics)
        current_voltage += control_signal * time_step
        
        predicted_voltage[0] = int(current_voltage) 
        
        predicted_data = {
            "point1": {
            "Inverter_ID": "1",
            "Measurement": "voltage",
            "Value": predicted_voltage[0],
        },
            "point2": {
            "Inverter_ID": "2",
            "Measurement": "voltage",
            "Value": predicted_voltage[0],
        },
            "point3": {
            "Inverter_ID": "3",
            "Measurement": "voltage",
            "Value": predicted_voltage[0],
        },
            "point4": {
            "Inverter_ID": "4",
            "Measurement": "voltage",
            "Value": predicted_voltage[0],
        }
            }
        
        for key in predicted_data:
            point = (
                Point("ML")
                .tag("Inverter_ID", predicted_data[key]["Inverter_ID"])
                .field(predicted_data[key]["Measurement"], predicted_data[key]["Value"]*1000)
                )
            client.write(database=database, record=point)
            time.sleep(0.1)

        print("Data Written to InfluxDB.")

        # Print results
        print(f"{time_elapsed:.2f}\t\t{current_voltage:.2f}\t\t{control_signal:.2f}")

        # Wait for the next time step
        time.sleep(time_step)
        time_elapsed += time_step



# Parameters
initial_voltage = influx_data[0] # Starting voltage (can be 200V or 250V)
print(f"initial voltage: {initial_voltage}")
target_voltage = (230 + initial_voltage)/10 # Desired voltage
print(f"target voltage: {target_voltage}")
Kp = 10            # Proportional gain (tune as needed)
Ki = 1              # Integral gain (tune as needed)
Kd = 0.05           # Derivative gain (tune as needed)

# Run simulation
val = simulate_inverter(initial_voltage, target_voltage, Kp, Ki, Kd)

print(f"val: {val}")
Kp = 7   
Ki = 0.25      
Kd = 3

simulate_inverter_increasing(val, 230, Kp, Ki, Kd)