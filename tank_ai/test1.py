# -*- coding: UTF-8 -*-
# Title: test1
# Time: 2018/4/11 1:38 PM
# Author: cijian
# Email: cijianzy@gmail.com

from websocket import create_connection
import json


import math

def rotate(x,y,theta):
    print(x * math.cos(theta) - y * math.sin(theta))
    print(x * math.sin(theta) + y * math.cos(theta))
    tx = x * math.cos(theta) - y * math.sin(theta)
    ty = x * math.sin(theta) + y * math.cos(theta)
    print(tx*tx/100 +ty*ty/100)


rotate(2,1,2.1415926)

