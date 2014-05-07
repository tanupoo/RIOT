#!/usr/bin/python

import networkx as nx
import matplotlib.pyplot as plt

def draw_graph(graph, labels=None, graph_layout='shell',
               node_size=1600, node_color='blue', node_alpha=0.3,
               node_text_size=12,
               edge_color='blue', edge_alpha=0.3, edge_tickness=1,
               edge_text_pos=0.3,
               text_font='sans-serif'):

    # create networkx graph
    G=nx.Graph()

    # add edges
    for edge in graph:
        G.add_edge(edge[0], edge[1])

    # these are different layouts for the network you may try
    # shell seems to work best
    if graph_layout == 'spring':
        graph_pos=nx.spring_layout(G)
    elif graph_layout == 'spectral':
        graph_pos=nx.spectral_layout(G)
    elif graph_layout == 'random':
        graph_pos=nx.random_layout(G)
    else:
        graph_pos=nx.shell_layout(G)

    # draw graph
    nx.draw_networkx_nodes(G,graph_pos,node_size=node_size, 
                           alpha=node_alpha, node_color=node_color)
    nx.draw_networkx_edges(G,graph_pos,width=edge_tickness,
                           alpha=edge_alpha,edge_color=edge_color)
    nx.draw_networkx_labels(G, graph_pos,font_size=node_text_size,
                            font_family=text_font)

    if labels is None:
        labels = range(len(graph))

    edge_labels = dict(zip(graph, labels))
    nx.draw_networkx_edge_labels(G, graph_pos, edge_labels=edge_labels, 
                                 label_pos=edge_text_pos)

    # show graph
    plt.show()

#graph = [('t9-113', 't9-124'), ('t9-113', 't9-108'), ('t9-113', 't9-117'), ('t9-117', 't9-124'), ('t9-117', 't9-113'), ('t9-124', 't9-108'), ('t9-124', 't9-105'), ('t9-124', 't9-117'), ('t9-124', 't9-106'), ('t9-124', 't9-113'), ('t9-108', 't9-124'), ('t9-108', 't9-105'), ('t9-108', 't9-106'), ('t9-108', 't9-113'), ('t9-169', 't9-105'), ('t9-169', 't9-165'), ('t9-169', 't9-166'), ('t9-105', 't9-169'), ('t9-105', 't9-124'), ('t9-105', 't9-108'), ('t9-105', 't9-106'), ('t9-105', 't9-165'), ('t9-105', 't9-166'), ('t9-165', 't9-169'), ('t9-165', 't9-166'), ('t9-165', 't9-164'), ('t9-164', 't9-108'), ('t9-164', 't9-106'), ('t9-164', 't9-165'), ('t9-164', 't9-166'), ('t9-106', 't9-124'), ('t9-106', 't9-108'), ('t9-106', 't9-105'), ('t9-106', 't9-164'), ('t9-166', 't9-169'), ('t9-166', 't9-165'), ('t9-166', 't9-164')]
# you may name your edge labels
#labels = ['1.00', '0.45', '1.00', '1.00', '1.00', '1.00', '1.00', '1.00', '0.73', '1.00', '0.99', '1.00', '1.00', '0.99', '0.48', '0.99', '1.00', '0.93', '1.00', '1.00', '1.00', '0.39', '0.35', '0.99', '1.00', '1.00', '1.00', '0.73', '0.99', '1.00', '0.04', '1.00', '1.00', '0.12', '1.00', '1.00', '1.00']
# if edge labels is not specified, numeric labels (0, 1, 2...) will be used
#draw_graph(graph, labels=labels, graph_layout="spring")
