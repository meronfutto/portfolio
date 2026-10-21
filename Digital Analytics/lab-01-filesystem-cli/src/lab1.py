import os  # работа с файлами и папками
import shutil  # копировать, перемещать файлы
import argparse  # принимать команды из терминала


def f_create(path):
    try:
        # Проверяем, что путь - строка
        if not isinstance(path, str):
            raise ValueError("Path must be a string")

        # Проверяем является ли поле пустым или содержит только пробелы
        if not path or not path.strip():
            raise ValueError("Path cannot be empty")

        if path.startswith('-'):
            raise ValueError("Path cannot start with '-'")

        # Проверяем, не является ли путь папкой (только если существует)
        if os.path.isdir(path):
            raise IsADirectoryError(f"Path '{path}' is a directory, not a file")

        # Создаём папки, если их нет
        dir_name = os.path.dirname(path)
        if dir_name:  # если не пустая строка
            os.makedirs(dir_name, exist_ok=True)

        # Создаём файл
        with open(path, 'w') as f:
            pass

        print(f"[+] File created successfully: {path}")

    except Exception as e:
        print(f"[-] Failed to create file '{path}': {e}")
        raise


def f_delete(path):
    try:

        if not isinstance(path, str):
            raise ValueError("Path must be a string")

        if not path or not path.strip():
            raise ValueError("Path cannot be empty")

        if path.startswith('-'):
            raise ValueError("Path cannot start with '-'")

        if os.path.isdir(path):

            raise IsADirectoryError(f"Path '{path}' is a directory, not a file")

        if not os.path.exists(path):

            raise FileNotFoundError("Path does not exist")


        if not os.path.isfile(path):

            raise IsADirectoryError("Path does not exist")

        os.remove(path)
        print(f"[+] File deleted successfully: {path}")

    except Exception as e:
        print(f"[-] Failed to delete file '{path}': {e}")
        raise


def f_write(path, content):
    try:
        if not isinstance(path, str):
            raise ValueError("Path must be a string")

        if not path or not path.strip():
            raise ValueError("Path cannot be empty")

        if path.startswith('-'):
            raise ValueError("Path cannot start with '-'")

        # Проверяем, не является ли путь папкой (только если существует)
        if os.path.isdir(path):
            raise IsADirectoryError(f"Path '{path}' is a directory, not a file")

        if content is None:
            raise ValueError("Content cannot be None")

        dir_name = os.path.dirname(path)
        if dir_name:
            os.makedirs(dir_name, exist_ok=True)

        with open(path, 'w', encoding='utf-8') as f:
            f.write(content)

        print(f"[+] File written successfully to: {path}")

    except Exception as e:
        print(f"[-] Failed to write to file '{path}': {e}")
        raise


def f_read(path):
    try:
        if not isinstance(path, str):
            raise ValueError("Path must be a string")

        if not path or not path.strip():
            raise ValueError("Path cannot be empty")

        if path.startswith('-'):
            raise ValueError("Path cannot start with '-'")


        if os.path.isdir(path):
            raise IsADirectoryError(f"Path '{path}' is a directory, not a file")

        if not os.path.exists(path):
            raise FileNotFoundError("Path does not exist")

        if not os.path.isfile(path):
            raise IsADirectoryError("Path does not exist")

        with open(path, 'r', encoding='utf-8') as f:
            content = f.read()

        print(f"[+] Content read successfully from: {path}")

        return content

    except Exception as e:
        print(f"[-] Failed to read file '{path}': {e}")
        raise


def f_copy(src, dest):
    try:
        # === ВАЛИДАЦИЯ src ===
        if not isinstance(src, str):
            raise ValueError("Path must be a string")

        if not src or not src.strip():
            raise ValueError("Path cannot be empty")

        if src.startswith('-'):
            raise ValueError("Path cannot start with '-'")

        if os.path.isdir(src):
            raise IsADirectoryError(f"Path '{src}' is a directory, not a file")

        if not os.path.exists(src):
            raise FileNotFoundError("Path does not exist")


        if not os.path.isfile(src):
            raise IsADirectoryError("Path does not exist")

        # === ВАЛИДАЦИЯ dest ===
        if not isinstance(dest, str):
            raise ValueError("Destination path must be a string")

        if not dest or not dest.strip():
            raise ValueError("Destination path cannot be empty")

        if dest.startswith('-'):
            raise ValueError("Destination path cannot start with '-'")

        # dest не должен быть существующей папкой
        if os.path.isdir(dest):
            raise IsADirectoryError(f"Destination path '{dest}' is a directory, not a file")

        # === ПОДГОТОВКА ПУТИ НАЗНАЧЕНИЯ === ( если нет )
        dest_dir = os.path.dirname(dest)
        if dest_dir:
            os.makedirs(dest_dir, exist_ok=True)

        # === КОПИРОВАНИЕ ===
        shutil.copy2(src, dest)

        print(f"[+] File copied successfully: {src} -> {dest}")

    except Exception as e:
        print(f"[-] Failed to copy file '{src}' to '{dest}': {e}")
        raise


def f_rename(src, dest):
    try:
        # === ВАЛИДАЦИЯ src ===
        if not isinstance(src, str):
            raise ValueError("Source path must be a string")

        if not src or not src.strip():
            raise ValueError("Source path cannot be empty")

        if src.startswith('-'):
            raise ValueError("Source path cannot start with '-'")


        if os.path.isdir(src):
            raise IsADirectoryError(f"Source path '{src}' is a directory, not a file")

        if not os.path.exists(src):
            raise FileNotFoundError(f"Source file '{src}' does not exist")

        if not os.path.isfile(src):
            raise IsADirectoryError("Path does not exist")

        # === ВАЛИДАЦИЯ dest ===
        if not isinstance(dest, str):
            raise ValueError("Destination path must be a string")

        if not dest or not dest.strip():
            raise ValueError("Destination path cannot be empty")

        if dest.startswith('-'):
            raise ValueError("Destination path cannot start with '-'")


        if os.path.isdir(dest):
            raise IsADirectoryError(f"Destination path '{dest}' is a directory, not a file")

        # === ПОДГОТОВКА ПУТИ НАЗНАЧЕНИЯ ===
        dest_dir = os.path.dirname(dest)
        if dest_dir:
            os.makedirs(dest_dir, exist_ok=True)

        # === ПЕРЕИМЕНОВАНИЕ ===
        os.rename(src, dest)

        print(f"[+] File renamed successfully: {src} -> {dest}")

    except Exception as e:
        print(f"[-] Failed to rename file '{src}' to '{dest}': {e}")
        raise


def main():
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(dest="command")

    create_parser = subparsers.add_parser("create")
    create_parser.add_argument("path")

    delete_parser = subparsers.add_parser("delete")
    delete_parser.add_argument("path")

    write_parser = subparsers.add_parser("write")
    write_parser.add_argument("path")
    write_parser.add_argument("content")

    read_parser = subparsers.add_parser("read")
    read_parser.add_argument("path")

    copy_parser = subparsers.add_parser("copy")
    copy_parser.add_argument("src")
    copy_parser.add_argument("dest")

    rename_parser = subparsers.add_parser("rename")
    rename_parser.add_argument("src")
    rename_parser.add_argument("dest")

    args = parser.parse_args()

    try:
        if args.command == "create":
            f_create(args.path)
        elif args.command == "delete":
            f_delete(args.path)
        elif args.command == "write":
            f_write(args.path, args.content)
        elif args.command == "read":
            result = f_read(args.path)
            if result is not None:
                print(result)
        elif args.command == "copy":
            f_copy(args.src, args.dest)
        elif args.command == "rename":
            f_rename(args.src, args.dest)
        else:
            parser.print_help()
    except Exception:
        exit(1)



main()