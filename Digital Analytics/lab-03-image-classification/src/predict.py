import torch
from torchvision import transforms
from PIL import Image
import argparse
from model import create_model


def predict_image(image_path, model_path='best_model.pth'):
    device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')

    # 1. Создаём архитектуру
    model = create_model(num_classes=2).to(device)

    # 2. Загружаем чекпоинт
    checkpoint = torch.load(model_path, map_location=device, weights_only=True)

    # Проверяем, как сохранена модель
    if isinstance(checkpoint, dict) and 'model_state_dict' in checkpoint:
        # Вариант 1: сохранён словарь {'model_state_dict': ...}
        model.load_state_dict(checkpoint['model_state_dict'])
    else:
        # Вариант 2: сохранён просто state_dict
        model.load_state_dict(checkpoint)

    model.eval()

    # 3. Препроцессинг
    transform = transforms.Compose([
        transforms.Resize((224, 224)),
        transforms.ToTensor(),
        transforms.Normalize(mean=[0.485, 0.456, 0.406], std=[0.229, 0.224, 0.225])
    ])

    image = Image.open(image_path).convert('RGB')
    image_tensor = transform(image).unsqueeze(0).to(device)

    # 4. Предсказание
    with torch.no_grad():
        outputs = model(image_tensor)
        _, predicted = torch.max(outputs.data, 1)

    classes = ['Животное', 'Человек']
    result = classes[predicted.item()]

    print(f"Файл: {image_path}")
    print(f"Результат: {result}")
    return result


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("image", type=str, help="Путь до картинки")
    args = parser.parse_args()

    predict_image(args.image)