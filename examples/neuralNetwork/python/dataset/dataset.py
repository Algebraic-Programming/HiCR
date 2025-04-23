import torch
import torchvision
from torchvision import datasets, transforms
import numpy as np
import os
import struct

# Define transformation to convert image to tensor
transform = transforms.Compose([transforms.ToTensor()])

# Load the MNIST test dataset
mnist_test = datasets.MNIST(root='./data', train=False, download=True, transform=transform)

# Create a directory to save the images (if it doesn't exist)
os.makedirs('./mnist_images_test', exist_ok=True)

# Open a file to write labels in binary format
with open('./mnist_images_test/labels.bin', 'wb') as label_file:
    # Loop through all images in the test dataset
    for idx, (image, label) in enumerate(mnist_test):
        # Convert the image to a numpy array (flattened)
        image_np = image.numpy().flatten()

        # Define file path for each image (e.g., "image_0.bin", "image_1.bin", etc.)
        file_path = f'./mnist_images_test/image_{idx}.bin'

        # Write the image data to a binary file
        with open(file_path, 'wb') as f:
            for value in image_np:
                f.write(struct.pack('f', value))  # 'f' format is for float32

        # Write the label to the labels.bin file as an unsigned int (32-bit)
        label_file.write(struct.pack('I', label))

        # Optionally, print out the progress every 100 images
        if idx % 100 == 0:
            print(f"Saved image {idx + 1}/{len(mnist_test)}")

print("All images and labels have been saved in binary format.")