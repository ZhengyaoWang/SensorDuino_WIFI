from http.server import BaseHTTPRequestHandler, HTTPServer
import json


class RequestHandler(BaseHTTPRequestHandler):
    def do_POST(self):
        content_length = int(self.headers['Content-Length'])  # Get the size of the data
        post_data = self.rfile.read(content_length)  # Read the data

        # Parse the incoming JSON data
        try:
            sensor_data = json.loads(post_data.decode('utf-8'))

            # Print the sensor data to the console
            print("Received sensor data:")
            print(f"Temperature: {sensor_data['temperature']} C")
            print(f"Humidity: {sensor_data['humidity']} %")
            print(f"Pressure: {sensor_data['pressure']} kPa")
            print(f"light: {sensor_data['light']} Lux")
            print(f"Gas: {sensor_data['gas']} %")
            print(f"CH2O: {sensor_data['CH2O']} mg/m3")
        except json.JSONDecodeError:
            print("Failed to decode JSON data")

        # Send a response back to the ESP32
        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()
        self.wfile.write(b"Data received successfully")


if __name__ == "__main__":
    server_address = ('', 8080)  # Listen on all available IP addresses, port 8080
    httpd = HTTPServer(server_address, RequestHandler)
    print("HTTP Server running on port 8080...")
    httpd.serve_forever()