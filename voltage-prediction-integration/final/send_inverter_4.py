import numpy as np
import pandas as pd
from sklearn.model_selection import train_test_split


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

predicted_voltage = np.array([230*1000])

predicted_data = {
    "point1": {
        "Inverter_ID": "2",
        "Measurement": "voltage",
        "Value": predicted_voltage[0],
    }
}

# Writing Data to InfluxDB
for key in predicted_data:
    point = (
        Point("Inverters")
        .tag("Inverter_ID", predicted_data[key]["Inverter_ID"])
        .field(predicted_data[key]["Measurement"], predicted_data[key]["Value"])
    )
    client.write(database=database, record=point)
    time.sleep(1)

print("Data Written to InfluxDB.")
print(f" predicted value: {predicted_voltage[0]}")


query_client.close()
client.close()
print("Process Complete. Return to InfluxDB UI.")
