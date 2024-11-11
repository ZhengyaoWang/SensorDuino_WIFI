import pandas as pd
from sqlalchemy import create_engine
from sklearn.model_selection import train_test_split
from sklearn.ensemble import RandomForestClassifier
from sklearn.metrics import accuracy_score
import numpy as np
import matplotlib.pyplot as plt

# Connect to the SQLite database
engine = create_engine('sqlite:///instance/sensor_data.db')
df = pd.read_sql('SELECT * FROM sensor_data', engine)

# Convert timestamp to datetime and calculate time difference
df['timestamp'] = pd.to_datetime(df['timestamp'])
df['time_diff'] = df['timestamp'].diff().dt.total_seconds().fillna(0)

# Extract rolling features (using the last 5 data points as an example)
df['temperature_roll'] = df['temperature'].rolling(window=5, min_periods=1).mean()
df['humidity_roll'] = df['humidity'].rolling(window=5, min_periods=1).mean()
df['pressure_roll'] = df['pressure'].rolling(window=5, min_periods=1).mean()
df['light_roll'] = df['light'].rolling(window=5, min_periods=1).mean()
df['tvoc_roll'] = df['tvoc'].rolling(window=5, min_periods=1).mean()
df['smoke_roll'] = df['smoke'].rolling(window=5, min_periods=1).mean()

# Select relevant features, including rolling features
features = df[['temperature_roll', 'humidity_roll', 'pressure_roll', 'light_roll', 'tvoc_roll', 'smoke_roll', 'time_diff']]

# Dictionary to store models and their predictions
models = {}
alarms = ['temp_alarm', 'humidity_alarm', 'pressure_alarm', 'tvoc_alarm', 'smoke_alarm']
predictions = {}

for alarm in alarms:
    # Define target variable
    target = df[alarm]

    # Split the data into training and testing sets
    X_train, X_test, y_train, y_test = train_test_split(features, target, test_size=0.2, random_state=42)

    # Train a Random Forest model for each alarm
    model = RandomForestClassifier(n_estimators=100, random_state=42)
    model.fit(X_train, y_train)

    # Save the model and evaluate accuracy
    models[alarm] = model
    y_pred = model.predict(X_test)
    print(f"{alarm} Model Accuracy: {accuracy_score(y_test, y_pred)}")

# Updated prediction function for the next 5 minutes (150 intervals of 2 seconds each)
def predict_future_alarms(data, models, prediction_intervals=150):
    predictions = {alarm: [] for alarm in models.keys()}

    for interval in range(prediction_intervals):
        # Create a synthetic future data point based on rolling averages
        future_data = pd.DataFrame(data.mean()).T
        future_data['time_diff'] = data['time_diff'].mean()  # Average time difference

        # Predict each alarm separately
        for alarm, model in models.items():
            alarm_prediction = model.predict(future_data)[0]
            predictions[alarm].append(alarm_prediction)

        # Update data with the new synthetic data point
        data = pd.concat([data, future_data]).iloc[1:]

    return predictions

# Use all data in the database for predictions
all_data = df[['temperature_roll', 'humidity_roll', 'pressure_roll', 'light_roll', 'tvoc_roll', 'smoke_roll', 'time_diff']]

# Predict alarm likelihood for each alarm type for the next 5 minutes using all data
predictions = predict_future_alarms(all_data, models)
print("Predicted alarm occurrences for the next 5 minutes for each alarm type:", predictions)

# Plot predictions for each alarm type in a single figure with subplots
fig, axes = plt.subplots(nrows=3, ncols=2, figsize=(15, 10))
fig.suptitle('Predicted Alarm Occurrences for the Next 5 Minutes')

# Define alarm labels for each subplot
alarm_labels = ['Temperature Alarm', 'Humidity Alarm', 'Pressure Alarm', 'TVOC Alarm', 'Smoke Alarm']

for i, (alarm, alarm_predictions) in enumerate(predictions.items()):
    row, col = divmod(i, 2)
    alarm_seconds = [2 * interval for interval, pred in enumerate(alarm_predictions) if pred == 1]
    axes[row, col].hist(alarm_seconds, bins=30, alpha=0.7, color='r')
    axes[row, col].set_xlabel('Time (Seconds)')
    axes[row, col].set_ylabel('Frequency')
    axes[row, col].set_title(f'Predicted {alarm_labels[i]}')

# Hide the last unused subplot if the number of alarms is odd
if len(alarms) % 2 != 0:
    axes[2, 1].axis('off')

plt.tight_layout(rect=[0, 0, 1, 0.95])
plt.show()
