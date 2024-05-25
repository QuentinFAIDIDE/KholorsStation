# Audio Transport Library

This library is meant to implement both clients and api servers
for clients to send audio with headers to the server. The Kholors II Station
will run the server to receive audio signals, and the Sinks will
run as VSTs to feed tracks to the station.

The current protocol for the server is gRPC, as it can give us more options
to optimize transport and content compression later on.
