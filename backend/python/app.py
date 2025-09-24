from flask import Flask

app = Flask(__name__)

@app.route("/")
def home():
    return "Flask backend is running!"

if __name__ == "__main__":
    #start_mqtt()       # start MQTT in background
    #start_scheduler()  # start periodic tasks
    app.run(host="0.0.0.0", port=5000)
