# Multi-consumer UDP-based deployment

`broadband` enables efficient mass deployment in a low-bandwidth environment, in contexts where download speed trumps other performance issues.

`broadband` provides what is effectively a NACK-based TCP implementation over broadcast UDP: all clients connect to the server at once and wait for the incoming stream; if at any point of time any client notices that it has lost a packet, it asks the server to rewind the stream so that data is not lost.


## Performance

While this seems rather inefficient, the performance of `broadband` quickly tops multiple TCP connections as you add more and more clients.

Suppose, for instance, that you need to deploy a 100 GB image to 100 machines connected to a server via Ethernet. Suppose also that due to various limitations, the server is connected to the network by 10 Gigabit Ethernet, and the packet loss rate is a generous 1% on each link.

With a normal TCP connection, performance decreases linearly: the 100 machines split the connection 100 ways, resulting in an effective 100 Megabit connection per machine. Accounting for packet loss we still get around 100 Mbps, so deployment will take 8000 seconds, or around 2 hours.

With broadcast UDP, however, performance only drops when a packet loss occurs. A packet is delivered to all 100 clients intact with  probability around 36%, which seems pretty low, because this effectively increases the image size almost three-fold, to staggering 280 GB; however, as the connection is no longer split between clients, and one packet from the server reaches all the clients immediately, the estimated deployment time is just under 4 minutes.


## Building

```shell
$ make
```


## Usage

On the server, run

```shell
$ ./server <broadcast address>
```

...where `<broadcast address>` is the broadcast address of the interface to which to stream the image from stdin. Note that stdin has to be rewindable, which usually means you have to stream it from a file.
	
On all the clients, run

```shell
$ ./client <server address>
```

This connects to the server at `<server address>` and streams the image to stdout. When the download is complete, the client exists normally.

It is better to run the client on all devices at once, as the server rewinds to the first chunk of the file that any of the clients don't have -- so if you start one client and then another, the server will rewind the file to the very beginning and send the very same data, which won't cause problems, but is inefficient. To track the deployment state, the server occasionally logs how far into the file it is currently streaming, in percent. It might also be useful to stream the data to `pv` on the clients.

You will need to open UDP ports 7854 and 7855 on the network if you're using a firewall.
