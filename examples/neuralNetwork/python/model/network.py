import os
import torch
import torch.nn as nn
import torch.optim as optim
from torchvision import datasets, transforms


class NeuralNetwork(nn.Module):
    def __init__(self):
        super().__init__()
        self.gemm1 = nn.Linear(28*28, 512)
        self.left_branch_gemm1 = nn.Linear(512, 200)
        self.left_branch_gemm2 = nn.Linear(200, 100)
        self.right_branch_gemm1 = nn.Linear(512, 100)
        self.gemm2 = nn.Linear(100, 10)

    def forward(self, x):
        x = torch.flatten(x, 1)
        x = torch.relu(self.gemm1(x))
        left = torch.relu(self.left_branch_gemm1(x))
        left = torch.relu(self.left_branch_gemm2(left))
        right = torch.relu(self.right_branch_gemm1(x))
        x = torch.add(left, right)
        x = self.gemm2(x)
        return x

if __name__ == "__main__":
    device = torch.device("cpu")

    transform = transforms.Compose([transforms.ToTensor()])
    train_loader = torch.utils.data.DataLoader(datasets.MNIST('./data', train=True, download=True, transform=transform),
                                            batch_size=64, shuffle=True)
    test_loader = torch.utils.data.DataLoader(datasets.MNIST('./data', train=False, transform=transform),
                                            batch_size=1000, shuffle=False)

    
    model = NeuralNetwork().to(device)
    print(model)
    criterion = nn.CrossEntropyLoss()
    optimizer = optim.SGD(model.parameters(), lr=0.01)

    for epoch in range(10):
        model.train()
        for batch_idx, (data, target) in enumerate(train_loader):
            data, target = data.to(device), target.to(device)
            optimizer.zero_grad()
            output = model(data)
            loss = criterion(output, target)
            loss.backward()
            optimizer.step()

        model.eval()
        test_loss = 0
        correct = 0
        with torch.no_grad():
            for data, target in test_loader:
                data, target = data.to(device), target.to(device)
                output = model(data)
                test_loss += criterion(output, target).item()
                pred = output.argmax(dim=1, keepdim=True)
                correct += pred.eq(target.view_as(pred)).sum().item()

        test_loss /= len(test_loader.dataset)
        accuracy = 100. * correct / len(test_loader.dataset)
        print(f'Epoch {epoch+1} - Test loss: {test_loss:.4f}, Accuracy: {accuracy:.2f}%')
    
    dummy_input = torch.randn(1, 1, 28, 28, device=device)
    # Create a directory to save the images (if it doesn't exist)
    os.makedirs('./model', exist_ok=True)
    torch.onnx.export(model, dummy_input, "./model/neural_network.onnx")
    print("Model saved as neural_network.onnx")
