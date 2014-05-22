#!/usr/bin/python

'''

./loganalyser.py | python -mjson.tool

'''

import glob, math
import os
import networkx as nx
import matplotlib.pyplot as plt
import json
import graph as g

entries = {}

#subset = ["t9-169", "t9-165", "t9-166", "t9-105", "t9-164", "t9-124", "t9-108", "t9-117", "t9-113"]
#blacklist = ["t9-105", "t9-106", "t9-136", "t9-146", "t9-162", "t9-108", "t9-147", "t9-137", "t9-158", "t9-169", "t9-124", "t9-117", "t9-157t"]
blacklist = [None]

def readlogs():
    for fname in glob.glob(".pyterm/*/*.log"):
        host = fname.split('/')[1]
        print(host)

        lines = [line.strip().split("#")[1] for line in open(fname)]
        #print(lines)

        res = [line for line in lines if ("400" in line) and ("$" in line)]
        if res == []:
            print("!!!!! " + host)
            continue
            
        res = res[0][: -1]
        
        #print(res)

        csv = [int(val.strip()) for val in res.split("$")[1].split(",")]
        nodeid = csv.pop(0)
        send = csv.pop(0)

        received = csv[:int(len(csv)/2)]
        real = csv[int(len(csv)/2):]

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
    
    #print(tuples)
    return None

if __name__ == '__main__':

    readlogs()

    tuples = []
    nxg = nx.Graph()

    for name in entries.keys():
        tuples.append((
            entries[name]["rate"],
            name,
            entries[name]["nodeid"]
        ))
        if not name in nxg.nodes():
            nxg.add_node(name)

    graph = []
    labels = []

    min_qual = 0.5
    min_degree = 10

    for rate, name, nodeid in tuples:
        for idx, val in enumerate(rate):
            if (val is not None) and (val > min_qual) and (name not in blacklist) and (lookup(tuples, idx) not in blacklist):
                graph.append((name, lookup(tuples, idx)))
                nxg.add_edge(name, lookup(tuples, idx))
                labels.append("%.2f" % val)

    for n in nxg.nodes():
#        print("Node %s has %d neighbors" % (n, nxg.neighbors(n)))
        if len(nxg.neighbors(n)) < min_degree:
            print("Remove node %s" % n)
            nxg.remove_node(n)

    for e in nxg.edges():
        if (not e[0] in nxg.nodes()) or (not e[1] in nxg.nodes()):
            print("Remove edge %s" % e)
            nxg.remove_edge(e)

    remove_elems = []
    for e in graph:
        print("checking %s" % str(e))
        if (not e[0] in nxg.nodes()) or (not e[1] in nxg.nodes()):
            print("Mark edge %s" % str(e))
            remove_elems.append(e)

    for re in remove_elems:
        print("Remove edge %s" % str(re))
        graph.remove(re)
    
    print("Nodes: %s" % nxg.nodes())
    print("Edges: %s" % nxg.edges())

    nr_sum = 0
    for n in nxg.nodes():
        nr_sum += len(nxg.neighbors(n))

    print("Average neighbor count: %d" % (nr_sum/len(nxg.nodes())))
    print(nxg.degree())
    hg = nx.degree_histogram(nxg)

    i = 0
    for e in hg:
        print("Degree %d: %d" % (i, e))
        i += 1

    graph_pos = nx.spring_layout(nxg, k=(1.5/math.sqrt(len(nxg.nodes()))), iterations=500)
    nx.draw_networkx_nodes(nxg, graph_pos, node_size=1200, alpha=0.3, node_color='green')
    nx.draw_networkx_edges(nxg, graph_pos, width=1, alpha=0.3, edge_color='blue')
    nx.draw_networkx_labels(nxg, graph_pos, font_size=12, font_family='sans-serif')

    edge_labels = dict(zip(graph, labels))
#    print("len(nxg.edges(): %d, len(labels): %d" % (len(nxg.edges()), len(labels)))
#    print("edge_labels: %s" % str(edge_labels))

    nx.draw_networkx_edge_labels(nxg, graph_pos, edge_labels=edge_labels, label_pos=0.3)

    plt.show()
    #g.draw_graph(graph, labels=labels, graph_layout="spring")
    
