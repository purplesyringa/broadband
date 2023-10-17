all: server client

server: server.c
	gcc $^ -o $@ -O2

client: client.c
	gcc $^ -o $@ -O2
