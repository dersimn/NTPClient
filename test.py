import time
import requests
import json

start = time.time()
r = requests.get(url='http://192.168.0.132/epoch')
delay = time.time() - start
est_time = start + delay/2

data = r.json()



