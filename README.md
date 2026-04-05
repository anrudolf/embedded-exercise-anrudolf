# Embedded exercise

A Zephyr RTOS embedded exercise including CAN communication. Since I do not have a development kit with a CAN controller, I am using the Native Simulator (native_sim) target platform.

By default, there is a `can_loopback0` device on native_sim which allows for sending and receiving CAN messages on its own. Furthermore, there is also a [SocketCAN on Native Simulator Snippet (socketcan-native-sim)](https://docs.zephyrproject.org/latest/snippets/socketcan-native-sim/README.html) Zephyr snippet so that the target firmware can communicate via SocketCAN on the host.

## Workspace initialization

Example workspace initialization (also see [Zephyr Getting Started Guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html)).

```
mkdir workspace
cd workspace
python3 -m venv .venv
source .venv/bin/activate
pip install west

git clone https://github.com/anrudolf/embedded-exercise-anrudolf.git
west init -l embedded-exercise-anrudolf
west update
```

## Standalone loopback example

Build with loopback mode enabled
```
west build --board native_sim app -- -DCONFIG_APP_CAN_LOOPBACK=y
```

Run the app on the host and notice that periodic messages are logged
```
$ ./build/zephyr/zephyr.exe 
uart connected to pseudotty: /dev/pts/5
uart_1 connected to pseudotty: /dev/pts/9
*** Booting Zephyr OS build v4.4.0-rc2-41-g149c8b1758a8 ***
(000500)       010   [8]  34 db 4d 76 a4 00 7d e2 
(001000)       020   [8]  78 41 27 fe ca bf cf 7b 
(001000)       010   [8]  91 35 c9 e1 bd df a5 fb 
(001500)       010   [8]  32 c9 2b b5 c0 f1 56 cf 
(002000)       030   [8]  ba 65 ed fa d1 c1 24 d3 
(002000)       020   [8]  ed 0b 9f f1 25 93 fe df
```

In the example above, there are two pseudottys on /dev/pts/5 and /dev/pts/9.
The first one is the Zephyr console and second is the Zephyr shell (see [app/boards/native_sim.conf](./app/boards/native_sim.conf) and [app/boards/native_sim.overlay](./app/boards/native_sim.overlay)).
The Zephyr shell has the `can` command enabled (from `CONFIG_CAN_SHELL=y`) which can be useful for sending and dumping CAN frames.

## SocketCAN example

Follow instructions from the [socketcan-native-sim](https://docs.zephyrproject.org/latest/snippets/socketcan-native-sim/README.html) snippet page to create network zcan0 on the host

```
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link property add dev vcan0 altname zcan0
sudo ip link set vcan0 up
```

Build app with SocketCAN snippet and run on host machine (also enables the optional START, STOP and HELLO messages).
```
$ west build --pristine --board native_sim -S socketcan-native-sim app -- \
-DCONFIG_CAN_PRINTER_START_MSG=y \
-DCONFIG_CAN_PRINTER_STOP_MSG=y \
-DCONFIG_CAN_PRINTER_HELLO_MSG=y

$ ./build/zephyr/zephyr.exe
...
```

On host machine, check with can-utils `candump` that periodic messages are sent from the app
```
$ candump zcan0
  vcan0  010   [8]  34 DB 4D 76 A4 00 7D E2
  vcan0  020   [8]  78 41 27 FE CA BF CF 7B
  vcan0  010   [8]  91 35 C9 E1 BD DF A5 FB
  vcan0  010   [8]  32 C9 2B B5 C0 F1 56 CF
  vcan0  030   [8]  BA 65 ED FA D1 C1 24 D3
  vcan0  020   [8]  ED 0B 9F F1 25 93 FE DF
  ...
```

On host machine, send a CAN frame and note that nothing gets printed in the app (yet).
```
$ cansend zcan0 123#AABBCC
```

Send print start message (default ID 040) so that printing is enabled...
```
$ cansend zcan0 040#
```

... and again send a frame and note that it is now printed in the app console.
```
$ cansend zcan0 321#CCBBAA
```

The default optional message IDs are:
- 0x040=START
- 0x050=STOP
- 0x060=HELLO

## Running tests

```
source ../zephyr/zephyr-env.sh
twister --platform native_sim -T tests
```

## Remarks

Here are some general remarks:
- The assignment said to print "timestamps". I am using kernel uptime timestamps, but now that I think about it,
  maybe the intention was to use CAN RX timestamps. These would need to be enabled with `CONFIG_CAN_RX_TIMESTAMP=y`.
- The CAN frames are printed depending on whether the start/stop (if defined) messages were received.
  The "hello specialized" message is always printed (this is how I interpreted the assignment but could be debated).
- Currently the CAN sender just creates random data with a fixed length of 8. The length could also be randomized.
- Apart from `CONFIG_APP_CAN_LOOPBACK` there are no options to configure the CAN peripheral, and only `CAN_MODE_NORMAL` is used.
  Could be improved by adding options for `CAN_MODE_FD`, etc.
- Currently, the TTY is only virtual, but with `native-tty-uart` it should be possible to connect and communicate with physical
  UART adapters on the host. See the [UART native TTY sample](https://github.com/zephyrproject-rtos/zephyr/tree/main/samples/drivers/uart/native_tty)
- I used printk for printing the messages, but of course the Logging API would be a good idea as well. With printk it's rather
  straightforward to print hex characters individually. With the Logging API one would first have to create a hex string
  (possibly with the [bin2hex()](https://docs.zephyrproject.org/latest/doxygen/html/group__sys-util.html#gaf8f2ab98cc3f045ba834dbbb13a5dfd7) utility)
- There is a [Controller Area Network (CAN) Host Tests](https://github.com/zephyrproject-rtos/zephyr/tree/main/tests/drivers/can/host) in Zephyr
  for testing CAN communication with pytest. It should also be possible to test the native_sim target firmware with SocketCAN. This would make for a great test!
