#!/usr/bin/python

from commands import *
import subprocess
from time import time
import sys
from datetime import *

#routers_file = "/home/mehlis/testbed/meshrouters.list"
routers_file = "/home/mehlis/testbed/small.list"
tty = "/dev/ttyUSB0"
file_path = "ccn-lite-client.hex"
run_name = str(time())

routers = [x.strip("\n") for x in open(routers_file, "r").readlines()]
count = len(routers)-1

print "start: " + str(datetime.time(datetime.now()))

processes = []
for r in routers:
	command = "ssh -oBatchMode=yes " + r + " -- sudo /usr/local/bin/lpc2k_pgm " + tty + " " + file_path
	#command = "ssh -oBatchMode=yes " + r + " -- sudo /home/mehlis/testbed/tools/pyterm.py -rn " + run_name
	print command

	p = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
	processes.append(p)

for p in processes:
	output,stderr = p.communicate()
	status = p.poll()
	print output

print "end: " + str(datetime.time(datetime.now()))

