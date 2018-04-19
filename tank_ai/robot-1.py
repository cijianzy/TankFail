# -*- coding: UTF-8 -*-
# Title: main
# Time: 2018/4/11 12:33 PM
# Author: cijian
# Email: cijianzy@gmail.com

#!/usr/bin/env python
#!/usr/bin/env python

# import asyncio
# import datetime
# import random
# import websockets
#
# async def time(websocket, path):
#     while True:
#         now = datetime.datetime.utcnow().isoformat() + 'Z'
#         await websocket.send(now)
#         await asyncio.sleep(random.random() * 3)
#
# start_server = websockets.serve(time, 'wss://tank-match.taobao.com/ai')
#
# asyncio.get_event_loop().run_until_complete(start_server)
# asyncio.get_event_loop().run_forever().run_forever()

#!/usr/bin/env python


import asyncio
import websockets
import json
import random
import sys
import math


time = 0


def getAngle(x,y,x1,y1):
    return math.atan2((y1-y),(x1-x));

async def hello(uri):

    global time
    connect_info = {'commandType': 'aiEnterRoom', 'roomId': 101550, 'accessKey': sys.argv[1], 'employeeId': int(sys.argv[2])}
    # connect_info = {'commandType': 'aiEnterRoom', 'roomId': 101550, 'accessKey': '96b69e5beaab0ac5dc5b6c8e9739d102', 'employeeId': 159806}

    print(json.dumps(connect_info))
    async with websockets.connect(uri) as websocket:
        await websocket.send(json.dumps(connect_info))
        while True:
            msg = await websocket.recv()
            msg = json.loads(msg)
            tanks = json.loads(msg['data'])['tanks'];
            # print(tanks)
            # print(msg['data']['tanks'])

            angle = getAngle(tanks[sys.argv[3]]['position'][0], tanks[sys.argv[3]]['position'][1], tanks['ai:58']['position'][0], tanks['ai:58']['position'][1])

            while(angle < 0) :
                angle += 2 * math.pi

            angle = angle * 360 / (2 * math.pi)

            await websocket.send(json.dumps({'commandType': 'fire'}))

            await websocket.send(json.dumps({'commandType': 'direction', 'angle': angle}))


            time += 1
            print(msg)




asyncio.get_event_loop().run_until_complete(
    hello('wss://tank-match.taobao.com/ai'))
