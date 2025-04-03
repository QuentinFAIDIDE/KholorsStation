# Headless Audio Broadcast

This C++ unit provides a Server and a Client that are meant
to run on all instances of the plugin.

The Server tries to start on a specific port across all instances
in a loop, in such a way that it is always started on one instance.
The client is started on all instances of the plugin as well.
It receives raw audio segments with tracks metadata from all clients
running across instances. Then it computes FFTs and exposes the resulting audio segments
and track metadata updates to be fetched by instances that have
the UI open with the visualizer.

The Server exposes an endpoint to fetch the new audio data (audio segments or track/daw metadata updates)
from the offset returned by the last call, and use a server identifier to potentially
ignore the offset if this is a new insteance of the server with different offset numbering that has
started since the last call to fetch audio data.
