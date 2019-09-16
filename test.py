import time
import requests
import json
import pandas as pd
import matplotlib.pyplot as plt

#url = 'http://192.168.0.132/epoch'
url = 'http://192.168.0.29/epoch'
errors = []
for i in range(40):
    start = time.time()
    r = requests.get(url=url)
    delay = time.time() - start
    est_time = start + delay/2
    data = r.json()
    err = {'client':data['epoch'], 'local':est_time, 'delay':delay, 'error':data['epoch'] - est_time}
    errors.append(err)
    print(err)
    time.sleep(15)

df = pd.DataFrame(errors)



