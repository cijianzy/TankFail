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

time = 0

async def hello(uri):

    global time
    connect_info = {'commandType': 'aiEnterRoom', 'roomId': 101550, 'accessKey': sys.argv[1], 'employeeId': int(sys.argv[2])}
    print(json.dumps(connect_info))
    async with websockets.connect(uri) as websocket:
        await websocket.send(json.dumps(connect_info))
        while True:
            msg = await websocket.recv()
            msg = json.loads(msg)

            await websocket.send(json.dumps({'commandType': 'fire'}))

            if time % random.randint(1,20) == 4:
                await websocket.send(json.dumps({'commandType': 'direction', 'angle': random.randint(0,360)}))
            else:
                pass

            time += 1
            print(msg)




asyncio.get_event_loop().run_until_complete(
    hello('wss://tank-match.taobao.com/ai'))