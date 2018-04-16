# -*- coding: UTF-8 -*-
# Title: test
# Time: 2018/4/11 12:54 PM
# Author: cijian
# Email: cijianzy@gmail.com
import websocket
import json
try:
    import thread
except ImportError:
    import _thread as thread
import time

def on_message(ws, message):
    print('message is ')
    print(message)

def on_error(ws, error):
    print(error)

def on_close(ws):
    print("### closed ###")

def on_recv(data, fin):
    print('recive')

def on_open(ws):
    def run(*args):

        connect_info = {'commandType': 'aiEnterRoom', 'roomId': 101550, 'accessKey': '248c641ededfc6d91bbc31bb2e2056ee', 'employeeId': 101550}
        print(json.dumps(connect_info))
        ws.send(json.dumps(connect_info))

        while True:
            time.sleep(1)
            ws.send("Hello %d" % i)
        time.sleep(1)
        print("thread terminating...")
    thread.start_new_thread(run, ())


if __name__ == "__main__":
    websocket.enableTrace(True)
    ws = websocket.WebSocketApp("wss://tank-match.taobao.com/ai",
                              on_message = on_message,
                              on_error = on_error,
                              on_cont_message=on_recv,
                              on_close = on_close)
    ws.on_open = on_open
    ws.run_forever()