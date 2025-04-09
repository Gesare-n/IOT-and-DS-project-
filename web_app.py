from flask import Flask, jsonify, render_template

app = Flask(__name__)

def read_log(file_name):
    """Utility function to read the last line of a log file."""
    try:
        with open(file_name, 'r') as file:
            lines = file.readlines()
        return lines[-1].strip() if lines else "No data"
    except Exception as e:
        return str(e)

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/data', methods=['GET'])
def get_data():
    temperature_data = read_log("/home/andreas/Html-Css-Website_Kilimo360/temp_log.txt")
    water_data = read_log("/home/andreas/Html-Css-Website_Kilimo360/water_log.txt")
    humidity_data = read_log("/home/andreas/Html-Css-Website_Kilimo360/humid_log.txt")
    moisture_data = read_log("/home/andreas/Html-Css-Website_Kilimo360/moist_log.txt")

    return jsonify({
        'temperature': temperature_data,
        'water': water_data,
        'humidity': humidity_data,
        'moisture': moisture_data
    })

if __name__ == '__main__':
    app.run(port=5001)  # Use a different port
