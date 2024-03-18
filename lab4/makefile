all: shared server client

shared:
	$(MAKE) -C shared

server: shared
	$(MAKE) -C server

client: shared
	$(MAKE) -C client

clean:
	$(MAKE) -C server clean
	$(MAKE) -C client clean
	$(MAKE) -C shared clean
