import matplotlib.pyplot as pl
import numpy as np
from matplotlib import projections

kernel_width = 5
sample = []

depths = np.zeros(kernel_width**2)

sum = 0

for i in range(kernel_width):
    j = i % (kernel_width // 2)
    if i > kernel_width // 2:
        i = j
    sample.append(i + 1)
    sum += i + 1

for i in range(len(sample)):
    sample[i] /= sum

kernel = []

sum = 0
for i in range(len(sample)):
    kernel.append([])
    for j in range(len(sample)):
        kernel[i].append(sample[i] * sample[j])
        depths[i * kernel_width + j] = kernel[i][j]
        sum += kernel[i][j]

_xx, _yy = np.meshgrid(np.arange(kernel_width), np.arange(kernel_width))
x, y = _xx.ravel(), _yy.ravel()

f = pl.figure(figsize=(8, 8))
ax = f.add_subplot(121, projection="3d")
ax.bar3d(x, y, np.zeros_like(depths), 1, 1, depths, shade=True)

pl.show()

print(sum)
print(str(kernel).replace("[", "{").replace("]", "}"))
print(str(sample).replace("[", "{").replace("]", "}"))
