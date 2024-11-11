import eventlet

# Monkey patch eventlet before importing anything else
eventlet.monkey_patch()

import threading
from flask import Flask, request, render_template
from flask_socketio import SocketIO, emit
from flask_sqlalchemy import SQLAlchemy
from flask_migrate import Migrate  # Use Flask-Migrate for future schema updates
from http.server import BaseHTTPRequestHandler, HTTPServer
import json
from datetime import datetime

app = Flask(__name__)

app.config['SECRET_KEY'] = 'secret!'
app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///sensor_data.db'  # SQLite database
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False
socketio = SocketIO(app, cors_allowed_origins="*")  # Allow cross-origin requests
db = SQLAlchemy(app)
migrate = Migrate(app, db)  # Initialize Flask-Migrate

# Define SensorData model with detailed alarm flags
class SensorData(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    temperature = db.Column(db.Float, nullable=False)
    humidity = db.Column(db.Float, nullable=False)
    pressure = db.Column(db.Float, nullable=False)
    light = db.Column(db.Float, nullable=False)
    tvoc = db.Column(db.Float, nullable=False)
    smoke = db.Column(db.Float, nullable=False)

    # Individual alarm flags for each sensor
    temp_alarm = db.Column(db.Integer, nullable=False, default=0)
    humidity_alarm = db.Column(db.Integer, nullable=False, default=0)
    pressure_alarm = db.Column(db.Integer, nullable=False, default=0)
    light_alarm = db.Column(db.Integer, nullable=False, default=0)
    tvoc_alarm = db.Column(db.Integer, nullable=False, default=0)
    smoke_alarm = db.Column(db.Integer, nullable=False, default=0)

    # Timestamp to indicate the live time for the data
    timestamp = db.Column(db.DateTime, nullable=False, default=datetime.utcnow)

# Create the database and tables
with app.app_context():
    db.create_all()

# Serve the HTML page
@app.route('/')
def index():
    return render_template('index.html')

# Endpoint to receive sensor data via HTTP POST
@app.route('/receive_data', methods=['POST'])
def receive_data():
    if request.method == 'POST':
        try:
            # Log incoming request details
            print(f"Headers: {request.headers}")
            print(f"Content Length: {request.content_length}")
            print(f"Raw Data: {request.data}")

            # Parse the incoming JSON data
            sensor_data = request.get_json()

            if sensor_data is None:
                print("No JSON data received.")
                return "Invalid data format", 400

            # Extract sensor data fields
            temperature = sensor_data.get('temperature')
            humidity = sensor_data.get('humidity')
            pressure = sensor_data.get('pressure')
            light = sensor_data.get('light', 0.0)  # Assuming light data might not be present
            tvoc = sensor_data.get('CH2O')  # Assuming CH2O corresponds to TVOC in mg/m3
            smoke = sensor_data.get('gas')

            # Log the received data
            print(f"Received sensor data: {sensor_data}")

            # Determine each sensor's alarm flag
            temp_alarm = 1 if temperature < 10 or temperature > 30 else 0
            humidity_alarm = 1 if humidity < 40 else 0
            pressure_alarm = 1 if pressure < 100 else 0
            light_alarm = 0  # No alarm limit for light as specified
            tvoc_alarm = 1 if tvoc > 0.1 else 0
            smoke_alarm = 1 if smoke > 35 else 0

            # Create new SensorData instance and add to the database
            try:
                with app.app_context():
                    new_data = SensorData(
                        temperature=temperature,
                        humidity=humidity,
                        pressure=pressure,
                        light=light,
                        tvoc=tvoc,
                        smoke=smoke,
                        temp_alarm=temp_alarm,
                        humidity_alarm=humidity_alarm,
                        pressure_alarm=pressure_alarm,
                        light_alarm=light_alarm,
                        tvoc_alarm=tvoc_alarm,
                        smoke_alarm=smoke_alarm,
                        timestamp=datetime.utcnow()
                    )
                    db.session.add(new_data)
                    db.session.commit()
                    print("Data successfully committed to the database")
            except Exception as db_error:
                print(f"Failed to commit data to the database: {db_error}")

            # Emit the data to all connected clients
            try:
                socketio.emit('sensor_data', {
                    'temperature': temperature,
                    'humidity': humidity,
                    'pressure': pressure,
                    'light': light,
                    'tvoc': tvoc,
                    'smoke': smoke
                })
                print("Emitting sensor data via WebSocket")
            except Exception as emit_error:
                print(f"Failed to emit data via WebSocket: {emit_error}")

            return "Data received successfully", 200
        except Exception as e:
            print(f"Failed to process data: {e}")
            return "Invalid data format", 400

# HTTP Server to receive data from sensor
class RequestHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        # Send a simple response for GET requests
        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()
        self.wfile.write(b"GET request handled successfully")

    def do_POST(self):
        try:
            # Log headers and content length for debugging
            print(f"Headers: {self.headers}")
            content_length = int(self.headers['Content-Length'])  # Get the size of the data
            print(f"Content Length: {content_length}")

            post_data = self.rfile.read(content_length)  # Read the data
            print(f"Raw Data: {post_data}")

            # Parse the incoming JSON data
            sensor_data = json.loads(post_data.decode('utf-8'))

            # Extract sensor data fields
            temperature = sensor_data.get('temperature')
            humidity = sensor_data.get('humidity')
            pressure = sensor_data.get('pressure')
            light = sensor_data.get('light', 0.0)  # Assuming light data might not be present
            tvoc = sensor_data.get('CH2O')  # Assuming CH2O corresponds to TVOC in mg/m3
            smoke = sensor_data.get('gas')

            # Log the received data
            print(f"Received sensor data: {sensor_data}")
            # Determine each sensor's alarm flag
            temp_alarm = 1 if temperature < 10 or temperature > 30 else 0
            humidity_alarm = 1 if humidity < 40 else 0
            pressure_alarm = 1 if pressure < 100 else 0
            light_alarm = 0  # No alarm limit for light as specified
            tvoc_alarm = 1 if tvoc > 0.1 else 0
            smoke_alarm = 1 if smoke > 35 else 0
            # Use Flask app context to add data to the database
            with app.app_context():
                new_data = SensorData(
                    temperature=temperature,
                    humidity=humidity,
                    pressure=pressure,
                    light=light,
                    tvoc=tvoc,
                    smoke=smoke,
                    temp_alarm=temp_alarm,
                    humidity_alarm=humidity_alarm,
                    pressure_alarm=pressure_alarm,
                    light_alarm=light_alarm,
                    tvoc_alarm=tvoc_alarm,
                    smoke_alarm=smoke_alarm,
                    timestamp=datetime.utcnow()
                )
                db.session.add(new_data)
                db.session.commit()
                print("Data successfully committed to the database")

            # Emit the data to all connected clients
            socketio.emit('sensor_data', {
                'temperature': temperature,
                'humidity': humidity,
                'pressure': pressure,
                'light': light,
                'tvoc': tvoc,
                'smoke': smoke
            })
            print("Emitting sensor data via WebSocket")

            # Send a response back to the sensor
            self.send_response(200)
            self.send_header('Content-type', 'text/html')
            self.end_headers()
            self.wfile.write(b"Data received successfully")
        except json.JSONDecodeError:
            print("Failed to decode JSON data")
            self.send_response(400)
            self.end_headers()

# Run the HTTP server in a separate thread with correct app context
def run_http_server():
    with app.app_context():
        server_address = ('', 8080)  # Listen on all available IP addresses, port 8080
        httpd = HTTPServer(server_address, RequestHandler)
        print("HTTP Server running on port 8080...")
        httpd.serve_forever()

@app.route('/get_latest_data', methods=['GET'])
def get_latest_data():
    try:
        # Query the latest sensor data entry from the database
        latest_data = SensorData.query.order_by(SensorData.id.desc()).first()

        if latest_data is None:
            return {"error": "No data available"}, 404

        # Create a dictionary to return as JSON
        data = {
            'temperature': latest_data.temperature,
            'humidity': latest_data.humidity,
            'pressure': latest_data.pressure,
            'light': latest_data.light,
            'tvoc': latest_data.tvoc,
            'smoke': latest_data.smoke,
            'timestamp': latest_data.timestamp.strftime("%Y-%m-%d %H:%M:%S")
        }

        return data, 200
    except Exception as e:
        print(f"Failed to retrieve data: {e}")
        return {"error": "Failed to retrieve data"}, 500

if __name__ == '__main__':
    threading.Thread(target=run_http_server, daemon=True).start()
    socketio.run(app, debug=True, host='0.0.0.0', port=5000)