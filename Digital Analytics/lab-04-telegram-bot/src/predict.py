import argparse
import io
import os

import torch
from PIL import Image
from torchvision import transforms

from model import create_model


DEVICE = torch.device("cuda" if torch.cuda.is_available() else "cpu")
MODEL_PATH = os.path.join(os.path.dirname(__file__), "best_model.pth")
MODEL = None


def load_model(model_path=MODEL_PATH):
    global MODEL

    if MODEL is not None:
        return MODEL

    model = create_model(num_classes=2).to(DEVICE)
    checkpoint = torch.load(model_path, map_location=DEVICE, weights_only=True)

    if isinstance(checkpoint, dict) and "model_state_dict" in checkpoint:
        model.load_state_dict(checkpoint["model_state_dict"])
    else:
        model.load_state_dict(checkpoint)

    model.eval()
    MODEL = model
    return MODEL


def predict_image(image_bytes, model_path=MODEL_PATH):
    model = load_model(model_path)

    transform = transforms.Compose([
        transforms.Resize((224, 224)),
        transforms.ToTensor(),
        transforms.Normalize(mean=[0.485, 0.456, 0.406],
                             std=[0.229, 0.224, 0.225])
    ])

    image = Image.open(io.BytesIO(image_bytes)).convert("RGB")
    image_tensor = transform(image).unsqueeze(0).to(DEVICE)

    with torch.no_grad():
        outputs = model(image_tensor)
        _, predicted = torch.max(outputs.data, 1)

    classes = ["Животное", "Человек"]
    return classes[predicted.item()]


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("image", type=str, help="Path to image")
    args = parser.parse_args()

    with open(args.image, "rb") as image_file:
        print(predict_image(image_file.read()))
