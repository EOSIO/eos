Utility Server Example Application Tutorial
===========================================

Introduction
------------

This tutorial provides a step by step discussion of building a basic WebSocket++ server. The final product of this tutorial is the utility_server example application from the example section. This server demonstrates the following features:

- Use Asio Transport for networking
- Accept multiple WebSocket connections at once
- Read incoming messages and perform a few basic actions (echo, broadcast, telemetry, server commands) based on the path
- Use validate handler to reject connections to invalid paths
- Serve basic HTTP responses with the http handler
- Gracefully exit the server
- Encrypt connections with TLS

This tutorial is current as of the 0.6.x version of the library.

Chapter 1: Initial Setup & Basics
---------------------------------

### Step 1

_Add WebSocket++ includes and set up a a server endpoint type._

WebSocket++ includes two major object types. The endpoint and the connection. The
endpoint creates and launches new connections and maintains default settings for
those connections. Endpoints also manage any shared network resources.

The connection stores information specific to each WebSocket session.

> **Note:** Once a connection is launched, there is no link between the endpoint and the connection. All default settings are copied into the new connection by the endpoint. Changing default settings on an endpoint will only affect future connections.
Connections do not maintain a link back to their associated endpoint. Endpoints do not maintain a list of outstanding connections. If your application needs to iterate over all connections it will need to maintain a list of them itself.

WebSocket++ endpoints are built by combining an endpoint role with an endpoint config. There are two different types of endpoint roles, one each for the client and server roles in a WebSocket session. This is a server tutorial so we will use the server role `websocketpp::server` which is provided by the `<websocketpp/server.hpp>` header.

> #### Terminology: Endpoint Config
> WebSocket++ endpoints have a group of settings that may be configured at compile time via the `config` template parameter. A config is a struct that contains types and static constants that are used to produce an endpoint with specific properties. Depending on which config is being used the endpoint will have different methods available and may have additional third party dependencies.

The endpoint role takes a template parameter called `config` that is used to configure the behavior of endpoint at compile time. For this example we are going to use a default config provided by the library called `asio`, provided by `<websocketpp/config/asio_no_tls.hpp>`. This is a server config that uses the Asio library to provide network transport and does not support TLS based security. Later on we will discuss how to introduce TLS based security into a WebSocket++ application, more about the other stock configs, and how to build your own custom configs.

Combine a config with an endpoint role to produce a fully configured endpoint. This type will be used frequently so I would recommend a typedef here.

`typedef websocketpp::server<websocketpp::config::asio> server`

#### `utility_server` constructor

This endpoint type will be the base of the utility_server object that will keep track of the state of the server. Within the `utility_server` constructor several things happen:

First, we adjust the endpoint logging behavior to include all error logging channels and all access logging channels except the frame payload, which is particularly noisy and generally useful only for debugging. [TODO: link to more information about logging]

~~~{.cpp}
m_endpoint.set_error_channels(websocketpp::log::elevel::all);
m_endpoint.set_access_channels(websocketpp::log::alevel::all ^ websocketpp::log::alevel::frame_payload);
~~~

Next, we initialize the transport system underlying the endpoint. This method is specific to the Asio transport not WebSocket++ core. It will not be necessary or present in endpoints that use a non-asio config.

> **Note:** This example uses an internal Asio `io_service` that is managed by the endpoint itself. This is a simple arrangement suitable for programs where WebSocket++ is the only code using Asio. If you have an existing program that already manages an `io_service` object or want to build a new program where WebSocket++ handlers share an io_service with other handlers you can pass the `io_service` you want WebSocket++ to register its handlers on to the `init_asio()` method and it will use it instead of generating and managing its own. [TODO: FAQ link instead?]

~~~{.cpp}
m_endpoint.init_asio();
~~~

#### `utility_server::run` method

In addition to the constructor, we also add a run method that sets up the listening socket, begins accepting connections, starts the Asio io_service event loop.

~~~{.cpp}
// Listen on port 9002
m_endpoint.listen(9002);

// Queues a connection accept operation
m_endpoint.start_accept();

// Start the Asio io_service run loop
m_endpoint.run();
~~~

The final line, `m_endpoint.run();`, will block until the endpoint is instructed to stop listening for new connections. While running it will listen for and process new connections as well as accept and process new data and control messages for existing connections. WebSocket++ uses Asio in an asyncronous mode where multiple connections can be similtaneously serviced efficiently within a single thread.

#### Build
Adding WebSocket++ has added a few dependencies to our program that must be addressed in the build system. Firstly, the WebSocket++ library headers need must be in the include search path of your build system. How exactly this is done depends on where you have the WebSocket++ headers installed what build system you are using.

For the rest of this tutorial we are going to assume a C++11 build environment. WebSocket++ will work with pre-C++11 systems if your build system has access to a recent version of the Boost library headers.

Finally, to use the Asio transport config we need to bring in the Asio library. There are two options here. If you have access to a C++11 build environment the standalone version from http://think-async.com is a good option. This header only library does not bring in any special dependencies and ensures you have the latest version of Asio. If you do not have a C++11 build environment or already have brought in the Boost libraries you can also use the version of Asio bundled with Boost.

To use standalone Asio, make sure the Asio headers are in your include path and define ASIO_STANDALONE. To use Boost Asio, make sure the Boost headers are in your include path and that you are linking to the boost_system library.

`c++ -std=c++11 step1.cpp` (Asio Standalone)
OR
`c++ -std=c++11 step1.cpp -lboost_system` (Boost Asio)

#### Code so far
```cpp
// The ASIO_STANDALONE define is necessary to use the standalone version of Asio.
// Remove if you are using Boost Asio.
#define ASIO_STANDALONE

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <functional>

typedef websocketpp::server<websocketpp::config::asio> server;

class utility_server {
public:
    utility_server() {
         // Set logging settings
        m_endpoint.set_error_channels(websocketpp::log::elevel::all);
        m_endpoint.set_access_channels(websocketpp::log::alevel::all ^ websocketpp::log::alevel::frame_payload);

        // Initialize Asio
        m_endpoint.init_asio();
    }

    void run() {
        // Listen on port 9002
        m_endpoint.listen(9002);

        // Queues a connection accept operation
        m_endpoint.start_accept();

        // Start the Asio io_service run loop
        m_endpoint.run();
    }
private:
    server m_endpoint;
};

int main() {
    utility_server s;
    s.run();
    return 0;
}
```

### Step 2

_Set up a message handler to echo all replies back to the original user_

#### Setting a message handler

> ###### Terminology: Registering handlers
> WebSocket++ provides a number of execution points where you can register to have a handler run. Which of these points are available to your endpoint will depend on its config. TLS handlers will not exist on non-TLS endpoints for example. A complete list of handlers can be found at  http://www.zaphoyd.com/websocketpp/manual/reference/handler-list.
>
> Handlers can be registered at the endpoint level and at the connection level. Endpoint handlers are copied into new connections as they are created. Changing an endpoint handler will affect only future connections. Handlers registered at the connection level will be bound to that specific connection only.
>
> The signature of handler binding methods is the same for endpoints and connections. The format is: `set_*_handler(...)`. Where * is the name of the handler. For example, `set_open_handler(...)` will set the handler to be called when a new connection is open. `set_fail_handler(...)` will set the handler to be called when a connection fails to connect.
>
> All handlers take one argument, a callable type that can be converted to a `std::function` with the correct count and type of arguments. You can pass free functions, functors, and Lambdas with matching argument lists as handlers. In addition, you can use `std::bind` (or `boost::bind`) to register functions with non-matching argument lists. This is useful for passing additional parameters not present in the handler signature or member functions that need to carry a 'this' pointer.
>
> The function signature of each handler can be looked up in the list above in the manual. In general, all handlers include the `connection_hdl` identifying which connection this even is associated with as the first parameter. Some handlers (such as the message handler) include additional parameters. Most handlers have a void return value but some (`validate`, `ping`, `tls_init`) do not. The specific meanings of the return values are documented in the handler list linked above.


### Step 3

_error handling_


### Step 4

_Set up open and close handlers and a connection data structure_

### Step 5

_Change the message handler for connections based on URI and add a validate handler to reject invalid URIs_

### Step 6

_Add some Admin commands (report total clients, cleanly shut down server)_

### Step 7

_Add some Broadcast commands_

### Step 8

_Add TLS_
