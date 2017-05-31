#!/usr/bin/python3
import requests
import sys
import os

while True:
    i = input()
    r = requests.post(os.environ["IFTTT_URL"], json={"value1":i}, headers={"Content-Type": "application/json", "User-Agent": "iodings v0.9"})
    if r.status_code != 200:
        print("POSTing notification resulted in HTTP code {}".format(r.status_code), file=sys.stderr)
    else:
        print("Successfully POSTed notification for '{}'".format(i))
