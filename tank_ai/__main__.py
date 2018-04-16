# -*- coding: UTF-8 -*-
# Title: __main__
# Time: 2018/4/11 12:15 PM
# Author: cijian
# Email: cijianzy@gmail.com

from websocket import create_connection
ws = create_connection("wss://tank-match.taobao.com/ai")
print("Sending 'Hello, World'...")
ws.send("Hello, World")
print("Sent")
print("Reeiving...")
result =  ws.recv()
print("Received '%s'" % result)
ws.close()
