from flask import Flask, request, render_template
from flask_socketio import SocketIO, emit
from flask_sqlalchemy import SQLAlchemy
from flask_migrate import Migrate  # Use Flask-Migrate for future schema updates

app = Flask(__name__)
app.config['SECRET_KEY'] = 'secret!'
app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///sensor_data.db'  # SQLite database
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False
socketio = SocketIO(app)
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
            # Parse the incoming JSON data
            sensor_data = request.get_json()

            # Extract sensor data fields
            temperature = sensor_data.get('temperature')
            humidity = sensor_data.get('humidity')
            pressure = sensor_data.get('pressure')
            light = sensor_data.get('light', 0.0)  # Assuming light data might not be present
            tvoc = sensor_data.get('CH2O')  # Assuming CH2O corresponds to TVOC in mg/m3
            smoke = sensor_data.get('gas')

            # Determine each sensor's alarm flag
            temp_alarm = 1 if temperature < 10 or temperature > 30 else 0
            humidity_alarm = 1 if humidity < 30 else 0
            pressure_alarm = 1 if pressure < 100 else 0
            light_alarm = 0  # No alarm limit for light as specified
            tvoc_alarm = 1 if tvoc > 0.1 else 0
            smoke_alarm = 1 if smoke > 50 else 0

            # Create new SensorData instance and add to the database
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
                smoke_alarm=smoke_alarm
            )
            db.session.add(new_data)
            db.session.commit()

            # Emit the data to all connected clients
            socketio.emit('sensor_data', {
                'temperature': temperature,
                'humidity': humidity,
                'pressure': pressure,
                'light': light,
                'tvoc': tvoc,
                'smoke': smoke
            })

            return "Data received successfully", 200
        except Exception as e:
            print(f"Failed to process data: {e}")
            return "Invalid data format", 400

# Emit data to client every 2 seconds and store in the database
@socketio.on('connect')
def handle_connect():
    print('Client connected')

if __name__ == '__main__':
    socketio.run(app, debug=True)
