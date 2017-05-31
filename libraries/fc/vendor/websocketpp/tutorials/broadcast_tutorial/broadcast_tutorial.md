Broadcast Tutorial
==================

This tutorial will dig into some more nitty gritty details on how to build high
scalability, high performance websocket servers for broadcast like workflows.

Will go into features like:
- minimizing work done in handlers
- using asio thread pool mode
- teaming multiple endpoints
- setting accept queue depth
- tuning compile time buffer sizes
- prepared messages
- flow control
- basic operating system level tuning, particularly increasing file descriptor limits.
- measuring performance with wsperf / autobahn
- tuning permessage-deflate compression settings