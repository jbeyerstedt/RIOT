# ota_update_app example

This example shows a minimal application, which uses the module for ota updates.

## Usage
Because this example uses Ethernet over serial (ethos), the ethos tool must
be used to establish a terminal connection.

### First Time Setup
Compile ethos for linux:
```
cd /dist/tools/ethos
make
```

### Establish the Ethernet Link
On Linux, setup a tap interface:
```
sudo ip tuntap add tap0 mode tap user ${USER}
sudo ip link set tap0 up
```

Then start the ethos de-multiplexing:
```
sudo ./dist/tools/ethos/ethos tap0 /dev/ttyACM0
```
The serial console of the application could be used in here.
