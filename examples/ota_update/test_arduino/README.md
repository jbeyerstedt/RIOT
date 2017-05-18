# test_arduino Readme

This arduino sketch is used in combination with the python test scripts to
remote control the power supply of the nucleo board.
The Seedstudio Relay shield v3 is used as a set of switches.

## Usage
The arduino sketch implements a simple and stateless protocol to control the
state of some predefined output pins.
Every control sequence sent to the arduino is answered.
All commands must be send by adding a newline `\n` character to the end of the transmitted string.
All commands are 9 charactes (plus the newline) long.

The UART communication uses the arduino default settings for parity and message
length.
The baudrate is 115200 baud.

### Commands

##### hello
First of all, there is a hello command to check, if a proper communication link
has been established.

```
Request:  HELLO
Response: ACK.HELLO
```

##### switch power relais
The second command is used to switch the relais, which will be used for
switching the power of the nucleo board on and off.

To switch the arduino pin on, send `PIN.PWR.1`:
```
Request:  PIN.PWR.1
Response: ACK.PWR.1
```

To switch the arduino pin off, send `PIN.PWR.0`:
```
Request:  PIN.PWR.0
Response: ACK.PWR.0
```


##### errors
If there was an error parsing a pin command, `ERR.PIN` is send as an answer.
For other errors, the error message is just `ERR`.


##### future extension
The protocol can be extended to use many different pins. But ther just wasn't
the need for more at the time.
