import requests
import csv
from datetime import datetime
import time

# Define the Blynk credentials and API URL
blynk_auth_token = 'YOUR_TOKEN'
base_url = 'https://blynk.cloud/external/api'

# Define the virtual pins corresponding to the data
virtual_pins = {
    'TEMPERATURA': 'V0',
    'UMIDADE': 'V1',
    'PRESSÃO RELATIVA': 'V2',
    'PRESSÃO ABSOLUTA': 'V3',
    'LUMINOSIDADE': 'V4',
    'TIMER': 'V5',
    'RSSI INTERNET_NODO': 'V6',
    'NODE_NETWORK_INFO': 'V7',
    'GATEWAY_NETWORK_INFO': 'V8',
    'RSSI INTERNET_GATEWAY': 'V9'
}

# Function to get data from Blynk for a given virtual pin
def get_blynk_data(auth_token, virtual_pin):
    url = f'{base_url}/get?token={auth_token}&pin={virtual_pin}'
    response = requests.get(url)
    try:
        return response.json()
    except requests.exceptions.JSONDecodeError:
        return response.text  # Return the raw text in case of JSON decode failure

# Function to fetch and save data to CSV
def fetch_and_save_data():
    # Fetch data for all virtual pins
    data = {}
    for metric, pin in virtual_pins.items():
        data[metric] = get_blynk_data(blynk_auth_token, pin)

    # Save data to CSV
    csv_file = 'blynk_data.csv'
    with open(csv_file, 'a', newline='') as csvfile:
        fieldnames = list(data.keys()) + ['timestamp']
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)

        # Write the header if the file is new
        if csvfile.tell() == 0:
            writer.writeheader()

        # Add a timestamp to the data
        data['timestamp'] = datetime.now().strftime('%Y-%m-%d %H:%M:%S')

        # Write the data
        writer.writerow(data)

# Main loop to read data every 10 seconds
while True:
    fetch_and_save_data()
    time.sleep(10)
