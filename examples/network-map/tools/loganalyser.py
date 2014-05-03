#!/usr/bin/python

'''

./loganalyser.py | python -mjson.tool

'''

import glob
import os
import networkx as nx
import matplotlib.pyplot as plt
import json

entries = {}


def readlogs():
    for fname in glob.glob(".pyterm/*/*.log"):
        host = fname.split('/')[1]

        lines = [line.strip().split("#")[1] for line in open(fname)]

        res = [line for line in lines if ("400" in line) and (len(line) > 100)][0][: -1]

        csv = [int(val.strip()) for val in res.split(",")]
        nodeid = csv.pop(0)
        send = csv.pop(0)

        received = csv[:len(csv)/2]
        real = csv[len(csv)/2:]

        rate = []
        for a, b in zip(received, real):
            if b != 0:
                rate.append(a/float(b))
            else:
                rate.append(0.0)

        entries[host] = {
            "nodeid": nodeid,
            "send": send,
            "received": received,
            "real": real,
            "rate": rate
        }

if __name__ == '__main__':

    readlogs()
    print(json.dumps(entries, ensure_ascii=False))
