# ota_update_server example

This example demonstrates the basic functions of an update server which answers
update requests from the the OTA Update example (`examples/ota_update_app`).

When CoAP block transfer (e.g. from the current version of libcoap) is
implemented in RIOT, this could also be used to devliver the update files
themself.

## Usage
First install go and the canopus library from the github repository
([github.com/zubairhamed/canopus](https://github.com/zubairhamed/canopus))

The server directory must be copied (or symlinked) to your go workspace
specified in the GOPATH environment variable.

To execute, or more precise; to link, the go application, the linker path must
be modified, because the openssl shared library will not be found in some cases.

```
LD_LIBRARY_PATH=$GOPATH/src/github.com/zubairhamed/canopus/openssl go run server.go
```

Just start the server and everything else will be done automatically.

To publish an update file, put the correspondig files in the same directory as
this go source file. This can be done by executing `make updatefiles-deploy`
in the `ota_update` folder.


#### install go
Download the go package with the package manager of your choise or follow the
instructions on the golang homepage.

All following examples will assume, that your go workspace is located in `~/go`
and that you set `GOPATH=~/go/` in your `.bashrc` or the equivalent for any
other shell used.

#### install the canopus coap library
The fully automatic installation via `go get` failed for me, so here is a step
by step solution:

```
go get -d github.com/zubairhamed/canopus
cd ~/go/src/github.com/zubairhamed/canopus/openssl
./config && make
cd ~/go
go fix github.com/zubairhamed/canopus
go install github.com/zubairhamed/canopus

cd src/github.com/zubairhamed/canopus/examples/simple
LD_LIBRARY_PATH=$GOPATH/src/github.com/zubairhamed/canopus/openssl go run server.go
```
