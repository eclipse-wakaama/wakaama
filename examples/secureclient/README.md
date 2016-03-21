This client requires tinyDTLS.
TinyDTLS is included as a GIT submodule. On first usage, you need to run the following commands to retrieve the sources:
git submodule init
git submodule update

You need to install the packages libtool and autoreconf.

In the wakaama/examples/utils/tinydtls run the following commands:
autoreconf -i
./configure

You can then build the secureclient using the commands:
cmake wakaama/examples/secureclient
make

