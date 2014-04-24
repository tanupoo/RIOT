#!/usr/bin/python

#routers_file = "./meshrouters.list"
routers_file = "/home/mehlis/testbed/small.list"

output_dir = "/home/mehlis/testbed/.pyterm"

routers = [x.strip("\n") for x in open(routers_file, "r").readlines()]

nodeid = 0
for r in routers:
	f = open(output_dir + "/" + r + ".conf", 'w')
	nodeid = nodeid + 1
	content = '[init_cmd]\ncmd=addr ' + str(nodeid) + '\n'
	f.write(content)
	f.close()

print "done: " + str(len(routers))
