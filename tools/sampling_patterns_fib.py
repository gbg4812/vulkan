import math
from random import sample

from PIL import Image, ImageColor, ImageDraw

search_samples = 8 
sample_samples = 80 
width = 100 

def worldToScreen(pos):
    [x, y] = pos
    pwidth = width - 4
    return [pwidth/2 + 2 + x * pwidth /2, pwidth/2 + 2 + y * pwidth / 2]
    
    

img = Image.new("RGB", [width, width])
draw = ImageDraw.Draw(img)

search = []
sample = []

for i in range(search_samples):
    a = (math.pi * 2 / search_samples) * i
    x = math.sin(a)
    y = math.cos(a)
    draw.point(worldToScreen([x, y]), ImageColor.getrgb("blue"))
    search.append([x, y])
    
for i in range(sample_samples):
    a = 2.39996322972865332 * i 
    depth = i * (1/sample_samples) 
    x = math.sin(a) * depth
    y = math.cos(a) * depth
    draw.point(worldToScreen([x, y]), ImageColor.getrgb("red"))
    sample.append([x, y])

print("const vec2 search_pattern[] = {", end="")
for [x, y] in search:
    print('{',f"{x}, {y}",'}', end=',')
print("};")

print("const vec2 sample_pattern[] = {", end="")
for [x, y] in sample:
    print('{',f"{x}, {y}",'}', end=',')
print("};")
img.show()
