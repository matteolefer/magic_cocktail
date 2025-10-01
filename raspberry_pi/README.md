# Raspberry Pi Flask Application

This directory hosts the Flask web interface that lets a Raspberry Pi drive the cocktail machine pumps through a USB serial link to an STM32 microcontroller.

## Features

* Web UI (Flask + Bootstrap) to choose a cocktail recipe from any browser.
* Recipes defined in `app.py` as pump proportions (three pumps supported by default).
* UART protocol that sends commands such as `1:4000` to ask the STM32 to run pump **1** for **4000&nbsp;ms**.
* Simple `/health` endpoint for monitoring.

## Requirements

* Python 3.10+
* [Flask](https://flask.palletsprojects.com/)
* [pyserial](https://pyserial.readthedocs.io/)

Install dependencies (ideally inside a virtual environment):

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install flask pyserial
```

## Usage

1. Edit the `DEFAULT_SERIAL_PORT` value in `app.py` if your STM32 board uses a different device node (e.g. `/dev/ttyUSB0`).
2. Start the server:

   ```bash
   FLASK_APP=app.py flask run --host=0.0.0.0 --port=5000
   ```

   Alternatively, run `python app.py` for development with live reload.
3. Open a browser and visit `http://<raspberry-ip>:5000`.
4. Pick a recipe and press **Préparer le cocktail**. The Flask backend sends sequential commands over UART to the STM32 controller.

## Adding New Recipes

Recipes live in the `RECIPES` dictionary (`app.py`). Each entry maps to pump proportions (float between 0 and 1). Ensure the values add up to 1.0 for consistent pour volumes.

```python
RECIPES = {
    "Margarita": {"pump1": 0.5, "pump2": 0.25, "pump3": 0.25},
}
```

## Dry-Run Without Hardware

If no STM32 is connected, Flask will raise a `SerialException`. You can comment out the call to `SerialPumpController.send_sequence` and log the sequence instead for UI demos.
