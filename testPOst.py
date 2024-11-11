import requests
import json

url = 'http://192.168.116.250:5000/receive_data'
headers = {'Content-Type': 'application/json'}
payload = {
    'temperature': 25.0,
    'humidity': 50.0,
    'pressure': 101.0,
    'light': 10.0,
    'CH2O': 0.05,
    'gas': 20.0
}

try:
    response = requests.post(url, json=payload, headers=headers)
    print(f"Response Status Code: {response.status_code}")
    print(f"Response Body: {response.text}")
except requests.exceptions.RequestException as e:
    print(f"Error during request: {e}")
