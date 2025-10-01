"""Flask application for controlling cocktail pumps via an STM32 controller."""

from pathlib import Path
from typing import Dict, List, Tuple

from flask import Flask, flash, redirect, render_template, request, url_for
import serial
import time

# Flask configuration
app = Flask(__name__)
app.secret_key = "magic-cocktail-secret-key"

# Serial configuration
DEFAULT_SERIAL_PORT = "/dev/ttyACM0"
SERIAL_BAUDRATE = 115200
SERIAL_TIMEOUT = 2.0  # seconds

# Pump configuration
PUMP_IDS = {
    "pump1": "1",
    "pump2": "2",
    "pump3": "3",
}
TOTAL_POUR_DURATION_S = 9.0
SERIAL_RETRY_DELAY_S = 0.5
SERIAL_MAX_RETRIES = 3

RECIPES: Dict[str, Dict[str, float]] = {
    "Mojito": {"pump1": 0.4, "pump2": 0.3, "pump3": 0.3},
    "Spritz": {"pump1": 0.2, "pump2": 0.5, "pump3": 0.3},
    "Tequila Sunrise": {"pump1": 0.5, "pump2": 0.3, "pump3": 0.2},
}

BASE_DIR = Path(__file__).resolve().parent
LOG_PATH = BASE_DIR / "serial.log"
LOG_PATH.parent.mkdir(parents=True, exist_ok=True)


def write_log(message: str) -> None:
    timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
    with LOG_PATH.open("a", encoding="utf-8") as log_file:
        log_file.write(f"{timestamp} - {message}\n")


class SerialPumpControllerError(Exception):
    """Base exception for serial pump controller errors."""


class SerialPumpTimeoutError(SerialPumpControllerError):
    """Raised when the STM32 does not respond before the timeout."""


class SerialPumpResponseError(SerialPumpControllerError):
    """Raised when the STM32 returns an error message."""


class SerialPumpController:
    """Wrapper around pyserial for sending pump activation commands."""

    def __init__(self, port: str = DEFAULT_SERIAL_PORT) -> None:
        self.port = port
        self._serial: serial.Serial | None = None

    def ensure_connection(self) -> None:
        if self._serial and self._serial.is_open:
            return
        self._serial = serial.Serial(
            self.port,
            SERIAL_BAUDRATE,
            timeout=SERIAL_TIMEOUT,
        )
        time.sleep(0.1)

    def _read_response(self) -> str:
        assert self._serial is not None
        response_bytes = self._serial.read_until(b"\n")
        if not response_bytes:
            raise SerialPumpTimeoutError("Aucune réponse du STM32 (délai dépassé)")
        response = response_bytes.decode("ascii", errors="ignore").strip()
        write_log(f"Reçu: {response}")
        if response == "OK":
            return response
        if response.upper().startswith("ERR"):
            raise SerialPumpResponseError(f"STM32 a renvoyé une erreur: {response}")
        raise SerialPumpResponseError(f"Réponse STM32 inattendue: {response}")

    def send_sequence(self, sequence: List[Tuple[str, float]]) -> None:
        self.ensure_connection()
        assert self._serial is not None

        for pump_id, duration_s in sequence:
            duration_ms = int(duration_s * 1000)
            command = f"{pump_id}:{duration_ms}\n"
            self._serial.write(command.encode("ascii"))
            self._serial.flush()
            write_log(f"Sent command {command.strip()} to STM32")
            self._read_response()

    def close(self) -> None:
        if self._serial and self._serial.is_open:
            self._serial.close()


controller = SerialPumpController()


def recipe_to_sequence(recipe: Dict[str, float]) -> List[Tuple[str, float]]:
    sequence: List[Tuple[str, float]] = []
    for pump_name, fraction in recipe.items():
        if pump_name not in PUMP_IDS:
            raise ValueError(f"Unknown pump '{pump_name}' in recipe")
        if fraction <= 0:
            continue
        duration = TOTAL_POUR_DURATION_S * fraction
        sequence.append((PUMP_IDS[pump_name], duration))
    return sequence


@app.route("/")
def index() -> str:
    return render_template("index.html", recipes=RECIPES)


@app.route("/prepare", methods=["POST"])
def prepare() -> str:
    recipe_name = request.form.get("recipe")
    if not recipe_name or recipe_name not in RECIPES:
        flash("Recette invalide", "error")
        return redirect(url_for("index"))

    recipe = RECIPES[recipe_name]
    sequence = recipe_to_sequence(recipe)
    flash_messages: List[Tuple[str, str]] = []

    for attempt in range(1, SERIAL_MAX_RETRIES + 1):
        try:
            controller.send_sequence(sequence)
            flash_messages.append((f"Commande envoyée pour {recipe_name}", "success"))
            for pump_name, fraction in recipe.items():
                duration = TOTAL_POUR_DURATION_S * fraction
                flash_messages.append(
                    (f"{pump_name} activée pendant {duration:.1f} s", "success")
                )
            flash_messages.append(("Cocktail prêt !", "success"))
            break
        except serial.SerialException as exc:  # type: ignore[attr-defined]
            error_message = f"Échec de la connexion série (tentative {attempt}): {exc}"
            write_log(error_message)
            if attempt == SERIAL_MAX_RETRIES:
                flash_messages.append(
                    ("Impossible de contacter le contrôleur STM32.", "error")
                )
            else:
                time.sleep(SERIAL_RETRY_DELAY_S)
        except SerialPumpTimeoutError as exc:
            write_log(str(exc))
            flash_messages.append((str(exc), "error"))
            break
        except SerialPumpResponseError as exc:
            write_log(str(exc))
            flash_messages.append((str(exc), "error"))
            break
        except ValueError as exc:
            flash_messages.append((str(exc), "error"))
            break

    for message, cat in flash_messages:
        flash(message, cat)

    return redirect(url_for("index"))


@app.route("/health")
def healthcheck() -> Dict[str, str]:
    return {"status": "ok", "recipes": list(RECIPES.keys())}


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=8000, debug=True)
