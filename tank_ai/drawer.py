# -*- coding: UTF-8 -*-
# Title: drawer
# Time: 2018/4/14 10:55 AM
# Author: cijian
# Email: cijianzy@gmail.com


import PIL
import sys


from PIL import Image, ImageDraw

im = Image.new('RGB',(25*20,15*20),'white')
draw = ImageDraw.Draw(im)

file = open('/Users/cijian/CLionProjects/tank/cmake-build-debug/blocks.txt', 'r');

file = file.readlines();

for line in file:
    cordi = line.split(',')
    x = int(cordi[0])
    y = int(cordi[1])
    print(x, y)
    if x >= 0 and x <= 25*20 and y >= 0 and y <= 15* 20:
        draw.point((x, y), 'black')

#
# file = open('/Users/cijian/CLionProjects/tank/cmake-build-debug/tanks.txt', 'r');
#
# file = file.readlines();
#
# for line in file:
#     cordi = line.split(',')
#     x = int(cordi[0])
#     y = int(cordi[1])
#     print(x, y)
#     if x >= 0 and x <= 25*20 and y >= 0 and y <= 15* 20:
#         draw.point((x, y), 'red')


file = open('/Users/cijian/CLionProjects/tank/cmake-build-debug/bulltes.txt', 'r');

file = file.readlines();

for line in file:
    cordi = line.split(',')
    x = int(float(cordi[0]));
    y = int(float(cordi[1]));
    print(x, y)
    if x >= 0 and x <= 25*20 and y >= 0 and y <= 15* 20:
        draw.point((x, y), 'red')
im.save("test.PNG", "PNG")
