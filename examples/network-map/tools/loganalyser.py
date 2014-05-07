#!/usr/bin/python

'''

./loganalyser.py | python -mjson.tool

'''

import glob
import os
import networkx as nx
import matplotlib.pyplot as plt
import json
import graph as g

entries = {}

#subset = ["t9-169", "t9-165", "t9-166", "t9-105", "t9-164", "t9-124", "t9-108", "t9-117", "t9-113"]
blacklist = ["t9-105", "t9-106", "t9-136", "t9-146", "t9-162", "t9-108", "t9-166", "t9-137", "t9-158", "t9-169", "t9-124", "t9-117", "t9-113"]

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
                rate.append(None)

        entries[host] = {
            "nodeid": nodeid,
            "send": send,
            "received": received,
            "real": real,
            "rate": rate
        }
        
def lookup(tuples, idx):
    for rate, name, nodeid in tuples:
        if nodeid == idx:
            return name
    return None

if __name__ == '__main__':

    readlogs()

    tuples = []
    for name in entries.keys():
        tuples.append((
            entries[name]["rate"],
            name,
            entries[name]["nodeid"]
        ))

    graph = []
    labels = []
    for rate, name, nodeid in tuples:
        for idx, val in enumerate(rate):
            if val is not None and (name not in blacklist) and (lookup(tuples, idx) not in blacklist):
                graph.append((name, lookup(tuples, idx)))
                labels.append("%.2f" % val)
    
    print(graph)
    print(labels)
    
    g.draw_graph(graph, labels=labels, graph_layout="spring")
    

