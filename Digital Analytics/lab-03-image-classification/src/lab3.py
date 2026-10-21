import torch
import torch.nn as nn
from torchvision import models


def create_model(num_classes=2):
    """
    Загружает предобученную ResNet-18 и меняет последний слой.
    """
    # 1. Загружаем "пустую" ResNet-18 с весами от ImageNet
    model = models.resnet18(weights=models.ResNet18_Weights.DEFAULT)

    # 2. Получаем количество входов последнего слоя (обычно 512)
    num_features = model.fc.in_features

    # 3. Заменяем последний слой на новый: 512 входов -> 2 выхода (Человек/Животное)
    model.fc = nn.Linear(num_features, num_classes)

    return model


if __name__ == "__main__":
    # Тест: создаем модель и выводим её структур
    model = create_model()
    print("Модель создана успешно!")
    print(f"Последний слой: {model.fc}")