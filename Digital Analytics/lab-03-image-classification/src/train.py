import torch
import torch.nn as nn
import torch.optim as optim
from torch.utils.data import DataLoader
from torchvision import datasets, transforms
from tqdm import tqdm
import argparse
import os
from model import create_model


def train(train_dir, val_dir, epochs=5, batch_size=32, lr=0.001, output_weights='best_model.pth'):
    """
    Обучает модель ResNet-18 на бинарную классификацию (Человек/Животное).
    Входы: пути к train и val датасетам.
    Выход: файл с весами обученной модели.
    """
    # Настройка устройства (GPU/CPU)
    device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
    print(f"[1] Используем устройство: {device}")

    # Трансформации данных (п.2 методички)
    transform = transforms.Compose([
        transforms.Resize((224, 224)),
        transforms.ToTensor(),
        transforms.Normalize(mean=[0.485, 0.456, 0.406], std=[0.229, 0.224, 0.225])
    ])

    # Загрузка датасетов из директорий
    print(f"[2] Загрузка данных...\n   Train: {train_dir}\n   Val:   {val_dir}")
    train_dataset = datasets.ImageFolder(train_dir, transform=transform)
    val_dataset = datasets.ImageFolder(val_dir, transform=transform)

    train_loader = DataLoader(train_dataset, batch_size=batch_size, shuffle=True)
    val_loader = DataLoader(val_dataset, batch_size=batch_size, shuffle=False)
    print(f"   Обнаружены классы: {train_dataset.classes}")

    # Инициализация модели, функции потерь и оптимизатора (п.5 методички)
    model = create_model(num_classes=2).to(device)
    criterion = nn.CrossEntropyLoss()
    optimizer = optim.Adam(model.parameters(), lr=lr)

    best_acc = 0.0

    # Цикл обучения
    for epoch in range(epochs):
        model.train()
        running_loss, correct, total = 0.0, 0, 0

        pbar = tqdm(train_loader, desc=f"Epoch {epoch+1}/{epochs} [Train]")
        for inputs, labels in pbar:
            inputs, labels = inputs.to(device), labels.to(device)

            optimizer.zero_grad()
            outputs = model(inputs)
            loss = criterion(outputs, labels)
            loss.backward()
            optimizer.step()

            running_loss += loss.item()
            _, predicted = torch.max(outputs.data, 1)
            total += labels.size(0)
            correct += (predicted == labels).sum().item()

        train_acc = 100 * correct / total

        # Валидация (п.6 методички)
        model.eval()
        val_correct, val_total = 0, 0
        with torch.no_grad():
            for inputs, labels in val_loader:
                inputs, labels = inputs.to(device), labels.to(device)
                outputs = model(inputs)
                _, predicted = torch.max(outputs.data, 1)
                val_total += labels.size(0)
                val_correct += (predicted == labels).sum().item()

        val_acc = 100 * val_correct / val_total
        print(f" Train Loss: {running_loss/len(train_loader):.4f} | Train Acc: {train_acc:.2f}% | Val Acc: {val_acc:.2f}%")

        # Сохраняем лучшую модель
        if val_acc > best_acc:
            best_acc = val_acc
            torch.save(model.state_dict(), output_weights)
            print(f"💾 Новые лучшие веса сохранены в: {output_weights} (Val Acc: {best_acc:.2f}%)")

    print(f"\n✅ ОБУЧЕНИЕ ЗАВЕРШЕНО.")
    print(f"📦 Вывод скрипта (файл весов): {os.path.abspath(output_weights)}")
    print(f"🎯 Итоговая точность на валидации: {best_acc:.2f}%")


if __name__ == "__main__":
    # Пункт 7: параметры подаются на вход скрипта
    parser = argparse.ArgumentParser(description="Обучение ResNet-18: Человек vs Животное")
    parser.add_argument("--train_dir", type=str, required=True, help="Путь до тренировочного датасета")
    parser.add_argument("--val_dir", type=str, required=True, help="Путь до валидационного/тестового датасета")
    parser.add_argument("--epochs", type=int, default=10, help="Количество эпох обучения")
    parser.add_argument("--batch_size", type=int, default=32, help="Размер батча")
    parser.add_argument("--lr", type=float, default=0.001, help="Learning rate")
    parser.add_argument("--output_weights", type=str, default="best_model.pth", help="Имя файла для сохранения весов")

    args = parser.parse_args()
    train(
        train_dir=args.train_dir,
        val_dir=args.val_dir,
        epochs=args.epochs,
        batch_size=args.batch_size,
        lr=args.lr,
        output_weights=args.output_weights
    )