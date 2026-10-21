import logging
import os
from typing import Dict, Optional

from flask import Flask, Response, request

try:
    import telebot
    from telebot import TeleBot
except ImportError:
    telebot = None
    TeleBot = None

from predict import predict_image


TOKEN = os.getenv("BOT_TOKEN", "123456:TEST_TOKEN")
WEBHOOK_URL = os.getenv("WEBHOOK_URL", "https://example.com/webhook")
PORT = 8080

logging.basicConfig(
    format="%(asctime)s - %(name)s - %(levelname)s - %(message)s",
    level=logging.INFO
)
logger = logging.getLogger(__name__)

user_storage: Dict[int, Dict[str, Optional[str | bool]]] = {}
user_states: Dict[int, Optional[str]] = {}
model = None

bot: Optional[TeleBot] = None
flask_app = None


class FallbackBot:
    def send_message(self, chat_id, text, **kwargs):
        print(f"send_message({chat_id}, {text})")

    def get_file(self, file_id):
        raise RuntimeError("telebot is not installed")

    def download_file(self, file_path):
        raise RuntimeError("telebot is not installed")

    def process_new_updates(self, updates):
        pass


def get_bot() -> TeleBot:
    global bot
    if bot is None:
        if TeleBot is None:
            bot = FallbackBot()
        else:
            bot = TeleBot(TOKEN)
            _register_handlers(bot)
    return bot


def get_chat_id(message):
    return message.chat.id


def get_user_id(message):
    return message.from_user.id if message.from_user else message.chat.id


def get_command_argument(message):
    parts = (message.text or "").split(maxsplit=1)
    return parts[1].strip() if len(parts) > 1 else ""


def get_user(user_id):
    if user_id not in user_storage:
        user_storage[user_id] = {
            "password": None,
            "authenticated": False
        }
    return user_storage[user_id]


def normalize_result(result):
    if result in ("Человек", "Human", "human", 1):
        return "Человек"
    if result in ("Животное", "Animal", "animal", 0):
        return "Животное"
    return str(result)


def handle_register(message):
    current_bot = get_bot()
    chat_id = get_chat_id(message)
    user_id = get_user_id(message)
    password = get_command_argument(message)

    if not password:
        user_states[user_id] = "waiting_register_password"
        current_bot.send_message(chat_id, "Введите пароль для регистрации")
        return

    user_storage[user_id] = {
        "password": password,
        "authenticated": False
    }
    user_states[user_id] = None
    current_bot.send_message(chat_id, "Регистрация выполнена")


def handle_login(message):
    current_bot = get_bot()
    chat_id = get_chat_id(message)
    user_id = get_user_id(message)
    user = get_user(user_id)
    password = get_command_argument(message)

    if user["password"] is None:
        current_bot.send_message(chat_id, "Сначала зарегистрируйтесь с помощью /register")
        return

    if not password:
        user_states[user_id] = "waiting_login_password"
        current_bot.send_message(chat_id, "Введите пароль")
        return

    if password == user["password"]:
        user["authenticated"] = True
        current_bot.send_message(chat_id, "Вы успешно авторизованы")
    else:
        user["authenticated"] = False
        current_bot.send_message(chat_id, "Неверный пароль")
    user_states[user_id] = None


def handle_logout(message):
    current_bot = get_bot()
    chat_id = get_chat_id(message)
    user_id = get_user_id(message)
    user = get_user(user_id)

    user["authenticated"] = False
    user_states[user_id] = None
    current_bot.send_message(chat_id, "Вы вышли из системы")


def handle_predict(message):
    current_bot = get_bot()
    chat_id = get_chat_id(message)
    user_id = get_user_id(message)
    user = get_user(user_id)

    if not user["authenticated"]:
        current_bot.send_message(chat_id, "Вы не авторизованы, сначала используйте /login")
        return

    if getattr(message, "photo", None):
        classify_photo(message)
        return

    user_states[user_id] = "waiting_photo"
    current_bot.send_message(chat_id, "Отправьте изображение для классификации")


def handle_photo(message):
    current_bot = get_bot()
    chat_id = get_chat_id(message)
    user_id = get_user_id(message)
    user = get_user(user_id)

    if not user["authenticated"]:
        current_bot.send_message(chat_id, "Вы не авторизованы, сначала используйте /login")
        return

    if user_states.get(user_id) != "waiting_photo":
        current_bot.send_message(chat_id, "Сначала используйте /predict")
        return

    classify_photo(message)


def classify_photo(message):
    current_bot = get_bot()
    chat_id = get_chat_id(message)
    user_id = get_user_id(message)

    try:
        photo = message.photo
        if not photo:
            current_bot.send_message(chat_id, "Фотография не найдена")
            return

        file_id = photo[-1].file_id
        file_info = current_bot.get_file(file_id)
        image_bytes = current_bot.download_file(file_info.file_path)
        result = normalize_result(predict_image(image_bytes))
        current_bot.send_message(chat_id, result)
    except Exception as error:
        logger.exception("Image classification error: %s", error)
        current_bot.send_message(chat_id, "Ошибка при обработке изображения")
    finally:
        user_states[user_id] = None


def handle_text(message):
    current_bot = get_bot()
    chat_id = get_chat_id(message)
    user_id = get_user_id(message)
    text = message.text or ""

    if user_states.get(user_id) == "waiting_register_password":
        if not text:
            current_bot.send_message(chat_id, "Пароль не может быть пустым")
            return
        user_storage[user_id] = {
            "password": text,
            "authenticated": False
        }
        user_states[user_id] = None
        current_bot.send_message(chat_id, "Регистрация выполнена")
        return

    if user_states.get(user_id) == "waiting_login_password":
        user = get_user(user_id)
        if text == user["password"]:
            user["authenticated"] = True
            current_bot.send_message(chat_id, "Вы успешно авторизованы")
        else:
            user["authenticated"] = False
            current_bot.send_message(chat_id, "Неверный пароль")
        user_states[user_id] = None
        return

    current_bot.send_message(chat_id, "Неизвестная команда")


def _register_handlers(telegram_bot: TeleBot) -> None:
    @telegram_bot.message_handler(commands=["start", "help"])
    def handle_start(message):
        help_text = (
            "Доступные команды:\n"
            "/register <пароль> - регистрация\n"
            "/login <пароль> - вход\n"
            "/logout - выход\n"
            "/predict - классификация изображения"
        )
        telegram_bot.send_message(message.chat.id, help_text)

    telegram_bot.message_handler(commands=["register"])(handle_register)
    telegram_bot.message_handler(commands=["login"])(handle_login)
    telegram_bot.message_handler(commands=["logout"])(handle_logout)
    telegram_bot.message_handler(commands=["predict"])(handle_predict)
    telegram_bot.message_handler(content_types=["photo"])(handle_photo)
    telegram_bot.message_handler(content_types=["text"])(handle_text)


def get_flask_app() -> Flask:
    global flask_app
    if flask_app is None:
        flask_app = Flask(__name__)
        _setup_routes(flask_app)
    return flask_app


def _setup_routes(app: Flask) -> None:
    if app is None:
        return

    @app.route(f"/{TOKEN}", methods=["POST"])
    def telegram_webhook():
        current_bot = get_bot()
        if telebot is None:
            return Response("Bot library not initialized", status=500)

        json_data = request.json
        if json_data:
            update = telebot.types.Update.de_json(json_data)
            current_bot.process_new_updates([update])
        return Response("OK")

    @app.route("/health", methods=["GET"])
    def healthcheck():
        return Response("OK")


if __name__ == "__main__":
    app = get_flask_app()
    app.run(port=PORT)
