# Standalone NovaDock Plugin SDK example for vbcc on Classic AmigaOS.
VC ?= vc
CFLAGS ?= -O -Iinclude -DND_PLUGIN_RESOURCES

all: Hello ArtHello

Hello: examples/hello.c src/nd_client.c src/nd_option_runtime.c include/nd_client.h include/nd_reply.h include/nd_protocol.h include/nd_option_runtime.h include/nd_artwork.h include/nd_plugin_support.h include/nd_plugin_resources.h src/nd_plugin_resources.c
	$(VC) $(CFLAGS) -o Hello examples/hello.c src/nd_client.c src/nd_option_runtime.c

ArtHello: examples/arthello.c src/nd_client.c include/nd_client.h include/nd_protocol.h include/nd_reply.h include/nd_artwork.h include/nd_plugin_support.h include/nd_plugin_resources.h src/nd_plugin_resources.c
	$(VC) $(CFLAGS) -o ArtHello examples/arthello.c src/nd_client.c

clean:
	-delete >NIL: Hello ArtHello


check:
	python3 tools/check_plugin.py .
