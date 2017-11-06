#pragma once
#include <memory>
#include <vector>

#include <boost/signals2.hpp>

#include <fc/variant.hpp>
#include <eosio/blockchain/transaction.hpp>

namespace eosio {

using namespace boost::signals2;
using namespace blockchain;
using namespace fc;
using namespace std;

struct connection_send_context;

class connection_interface {
   public:
      //Query if the connection is disconnected as well as a coresponding signals when the connection
      // becomes disconnected.
      //Once a connection becomes disconnected references to the particular connection must be reliniqushed
      // in short order. (a connection can never come alive again)
      virtual bool disconnected() = 0;
      virtual connection on_disconnected(const signal<void()>::slot_type& slot) = 0;

      //When called, a connection_interface must (if it's not already) begin to repeatedly
      //call network_plugin's get_work_to_send() parroting back the opaque context type.
      //This must be a NOOP if the connection has become disconnected.
      virtual void begin_processing_network_send_queue(connection_send_context& context) = 0;

      virtual ~connection_interface() {};
};

////The various types of work that can be asked of a connection_interface when querying for data to send///
//Not really work -- but an indicator the connection has done something that has caused
// it to irreversably fail. Connection should be disconnected immediately.
struct connection_send_failure {};

//There is no work to send (connection must quit asking for get_work_to_send until begin_processing_network_send_queue
// is called again)
struct connection_send_nowork {};

//Send the trasactions between begin-end and begin2-end2
struct connection_send_transactions {
   vector<meta_transaction_ptr>::iterator begin;
   vector<meta_transaction_ptr>::iterator end;
   vector<meta_transaction_ptr>::iterator begin2;
   vector<meta_transaction_ptr>::iterator end2;
};

using connection_send_work = static_variant<
   connection_send_failure,
   connection_send_nowork,
   connection_send_transactions
>;

}