import sqlite3
import paho.mqtt.client as mqtt
import time
from flask import Flask, jsonify

# MQTT Configuration
BROKER = "192.168.1.136"
PORT = 1883
TOPIC_TEMPERATURE = "temperature"
TOPIC_HUMIDITY = "humidity"
DB_FILE = "sensor_data.db"

# Flask API Setup
app = Flask(__name__)

# Connect to SQLite database
def connect_db():
    conn = sqlite3.connect(DB_FILE, check_same_thread=False)
    cursor = conn.cursor()
    cursor.execute('''
    CREATE TABLE IF NOT EXISTS sensor_data (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        timestamp TEXT DEFAULT (strftime('%Y-%m-%d %H:%M:%S', 'now', 'localtime')),
        topic TEXT,
        value REAL
    )
    ''')
    conn.commit()
    return conn, cursor

conn, cursor = connect_db()

# MQTT Callback - When a message is received
def on_message(client, userdata, msg):
    print(f"Received '{msg.payload.decode()}' on topic '{msg.topic}'")
    try:
        value = float(msg.payload.decode())
        timestamp = time.strftime('%Y-%m-%d %H:%M:%S')  # Get current time in proper format
        cursor.execute("INSERT INTO sensor_data (timestamp, topic, value) VALUES (?, ?, ?)", 
                       (timestamp, msg.topic, value))
        conn.commit()
    except ValueError:
        print("Invalid data received")

# MQTT client setup
client = mqtt.Client()
client.on_message = on_message
client.connect(BROKER, PORT, 60)
client.subscribe(TOPIC_TEMPERATURE)
client.subscribe(TOPIC_HUMIDITY)

# Run MQTT in background
client.loop_start()

# Flask API Routes for Grafana
@app.route('/data', methods=['GET'])
def get_data():
    cursor.execute("SELECT timestamp, topic, value FROM sensor_data ORDER BY timestamp DESC LIMIT 50")
    data = cursor.fetchall()
    return jsonify(data)

if __name__ == '__main__':
    try:
        print("Starting Flask API for Grafana")
        app.run(host='0.0.0.0', port=8080, debug=True)
    except KeyboardInterrupt:
        print("Shutting down...")
        client.loop_stop()
        client.disconnect()
        conn.close()
