#pragma once
#include <memory>
#include <boost/signals2.hpp>

namespace eos {

using namespace boost::signals2;

class connection_interface {
public:
   //Query if the connection is disconnected as well as a coresponding signals when the connection
   // becomes disconnected.
   //Once a connection becomes disconnected references to the particular connection must be reliniqushed
   // in short order. (a connection can never come alive again)
   virtual bool disconnected() = 0;
   virtual connection connect_on_disconnected(const signal<void()>::slot_type & slot) = 0;

   //get peer identify. Always valid once connection is given to network_manager

   //Send a transaction to the peer on this connection should it not already know of the transaction.
   //NOOP on disconnected connection.

   //Send a block to the peer on this connection
   //NOOP on disconnected connection.

};

using connection_interface_ptr = std::shared_ptr<connection_interface>;
}