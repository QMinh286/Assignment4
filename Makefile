.PHONY: all clean dc ds

all: dc ds

dc:
	$(MAKE) -C chat-client -f makeClient

ds:
	$(MAKE) -C chat-server -f makeServer

clean:
	$(MAKE) -C chat-client -f makeClient clean
	$(MAKE) -C chat-server -f makeServer clean