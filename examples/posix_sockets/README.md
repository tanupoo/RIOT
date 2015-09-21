examples/posix_sockets
======================
This application is a showcase for RIOT's POSIX socket support. To
keep things simple this application has only one-hop support and
no routing capabilities.

Usage
=====

Build, flash and start the application:
```
export BOARD=your_board
make
make flash
make term
```

The `term` make target starts a terminal emulator for your board. It
connects to a default port so you can interact with the shell, usually
that is `/dev/ttyUSB0`. If your port is named differently, the
`PORT=/dev/yourport` variable can be used to override this.


Example output
==============

The shell commands come with online help. Call `help` to see which commands
exist and what they do.

Running the `help` command on an iotlab-m3:
```
2014-05-06 13:14:38,508 - INFO # > help
```

Running the `ps` command on an iotlab-m3:

```
2014-05-09 17:38:33,388 - INFO # > ps
```

Start a UDP server:

```
> udp server start <port>
```

Stop a UDP server:

```
> udp server start <port>
```


Send a UDP package:

```
> udp send <ip_addr> <port> <content>
```
