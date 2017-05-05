# ota_update Example Automated Tests

The `tests.py` script contains some integration tests for the whole OTA Update
procedure.
Sadly it must be mentioned, that these test scripts are a bit instable regarding
the serial communication ports used.

## Usage
The test script can be customized by some command line options.

##### compiling the needed sources
Because these tests need quite a lot of different versions of the ota_update_app
it is recommended to compile these once and then do the tests without recompiling every time.

Setting the command line flag `-p` (prepare) will only compile all binaries.

The flag `-f` (fast) leads to the opposite behaviour, stepping over all the
compiling tasks and only executing the tests.

##### selecting individual tests
It it not needed to conduct every test every time.
With the flag `-t <num>` only the test with the number specified in `<num>` is
executed.
This flag can be used multiple times to specify multiple tests e.g. `-t 1 -t 2`
for doing test 1 and 2 only.

##### additional output
By default, the output of the flashing and some make operations is piped to
`/dev/null`, because including it in the tests script output will be quite
messy.
But with the flag `-o <output>` these contents could be redirected to some
other location.
