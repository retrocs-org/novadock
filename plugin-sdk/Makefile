# Standalone NovaDock Plugin SDK example for vbcc on Classic AmigaOS.
VC ?= vc
CFLAGS ?= -O -Iinclude

all: Hello

Hello: examples/hello.c src/nd_client.c src/nd_option_runtime.c include/nd_client.h include/nd_reply.h include/nd_protocol.h include/nd_option_runtime.h
	$(VC) $(CFLAGS) -o Hello examples/hello.c src/nd_client.c src/nd_option_runtime.c

clean:
	-delete >NIL: Hello

