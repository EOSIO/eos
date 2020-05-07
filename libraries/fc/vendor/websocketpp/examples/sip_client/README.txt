

Checkout the project from git

At the top level, run cmake:

  cmake -G 'Unix Makefiles' \
     -D BUILD_EXAMPLES=ON \
     -D WEBSOCKETPP_ROOT=/tmp/cm1 \
     -D ENABLE_CPP11=OFF .

and then make the example:

  make -C examples/sip_client

Now run it:

  bin/sip_client ws://ws-server:80

It has been tested against the repro SIP proxy from reSIProcate

  http://www.resiprocate.org/WebRTC_and_SIP_Over_WebSockets
