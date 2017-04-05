/*
 * Copyright (c) 2017, Respective Authors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <eos/net/peer_connection.hpp>
#include <eos/net/exceptions.hpp>
#include <eos/net/config.hpp>
#include <eos/chain/config.hpp>

#include <fc/thread/thread.hpp>

#ifdef DEFAULT_LOGGER
# undef DEFAULT_LOGGER
#endif
#define DEFAULT_LOGGER "p2p"

#ifndef NDEBUG
# define VERIFY_CORRECT_THREAD() assert(_thread->is_current())
#else
# define VERIFY_CORRECT_THREAD() do {} while (0)
#endif

namespace eos { namespace net
  {
    message peer_connection::real_queued_message::get_message(peer_connection_delegate*)
    {
      if (message_send_time_field_offset != (size_t)-1)
      {
        // patch the current time into the message.  Since this operates on the packed version of the structure,
        // it won't work for anything after a variable-length field
        std::vector<char> packed_current_time = fc::raw::pack(fc::time_point::now());
        assert(message_send_time_field_offset + packed_current_time.size() <= message_to_send.data.size());
        memcpy(message_to_send.data.data() + message_send_time_field_offset,
               packed_current_time.data(), packed_current_time.size());
      }
      return message_to_send;
    }
    size_t peer_connection::real_queued_message::get_size_in_queue()
    {
      return message_to_send.data.size();
    }
    message peer_connection::virtual_queued_message::get_message(peer_connection_delegate* node)
    {
      return node->get_message_for_item(item_to_send);
    }

    size_t peer_connection::virtual_queued_message::get_size_in_queue()
    {
      return sizeof(item_id);
    }

    peer_connection::peer_connection(peer_connection_delegate* delegate) :
      _node(delegate),
      _message_connection(this),
      _total_queued_messages_size(0),
      direction(peer_connection_direction::unknown),
      is_firewalled(firewalled_state::unknown),
      our_state(our_connection_state::disconnected),
      they_have_requested_close(false),
      their_state(their_connection_state::disconnected),
      we_have_requested_close(false),
      negotiation_status(connection_negotiation_status::disconnected),
      number_of_unfetched_item_ids(0),
      peer_needs_sync_items_from_us(true),
      we_need_sync_items_from_peer(true),
      inhibit_fetching_sync_blocks(false),
      transaction_fetching_inhibited_until(fc::time_point::min()),
      last_known_fork_block_number(0),
      firewall_check_state(nullptr)
#ifndef NDEBUG
      ,_thread(&fc::thread::current()),
      _send_message_queue_tasks_running(0)
#endif
    {
    }

    peer_connection_ptr peer_connection::make_shared(peer_connection_delegate* delegate)
    {
      // The lifetime of peer_connection objects is managed by shared_ptrs in node.  The peer_connection
      // is responsible for notifying the node when it should be deleted, and the process of deleting it
      // cleans up the peer connection's asynchronous tasks which are responsible for notifying the node
      // when it should be deleted.
      // To ease this vicious cycle, we slightly delay the execution of the destructor until the
      // current task yields.  In the (not uncommon) case where it is the task executing
      // connect_to or read_loop, this allows the task to finish before the destructor is forced
      // to cancel it.
      return peer_connection_ptr(new peer_connection(delegate));
      //, [](peer_connection* peer_to_delete){ fc::async([peer_to_delete](){delete peer_to_delete;}); });
    }

    void peer_connection::destroy()
    {
      VERIFY_CORRECT_THREAD();

#if 0 // this gets too verbose
#ifndef NDEBUG
      struct scope_logger {
        fc::optional<fc::ip::endpoint> endpoint;
        scope_logger(const fc::optional<fc::ip::endpoint>& endpoint) : endpoint(endpoint) { dlog("entering peer_connection::destroy() for peer ${endpoint}", ("endpoint", endpoint)); }
        ~scope_logger() { dlog("leaving peer_connection::destroy() for peer ${endpoint}", ("endpoint", endpoint)); }
      } send_message_scope_logger(get_remote_endpoint());
#endif
#endif

      try
      {
        dlog("calling close_connection()");
        close_connection();
        dlog("close_connection completed normally");
      }
      catch ( const fc::canceled_exception& )
      {
        assert(false && "the task that deletes peers should not be canceled because it will prevent us from cleaning up correctly");
      }
      catch ( ... )
      {
        dlog("close_connection threw");
      }

      try
      {
        dlog("canceling _send_queued_messages task");
        _send_queued_messages_done.cancel_and_wait(__FUNCTION__);
        dlog("cancel_and_wait completed normally");
      }
      catch( const fc::exception& e )
      {
        wlog("Unexpected exception from peer_connection's send_queued_messages_task : ${e}", ("e", e));
      }
      catch( ... )
      {
        wlog("Unexpected exception from peer_connection's send_queued_messages_task");
      }

      try
      {
        dlog("canceling accept_or_connect_task");
        accept_or_connect_task_done.cancel_and_wait(__FUNCTION__);
        dlog("accept_or_connect_task completed normally");
      }
      catch( const fc::exception& e )
      {
        wlog("Unexpected exception from peer_connection's accept_or_connect_task : ${e}", ("e", e));
      }
      catch( ... )
      {
        wlog("Unexpected exception from peer_connection's accept_or_connect_task");
      }

      _message_connection.destroy_connection(); // shut down the read loop
    }

    peer_connection::~peer_connection()
    {
      VERIFY_CORRECT_THREAD();
      destroy();
    }

    fc::tcp_socket& peer_connection::get_socket()
    {
      VERIFY_CORRECT_THREAD();
      return _message_connection.get_socket();
    }

    void peer_connection::accept_connection()
    {
      VERIFY_CORRECT_THREAD();

      struct scope_logger {
        scope_logger() { dlog("entering peer_connection::accept_connection()"); }
        ~scope_logger() { dlog("leaving peer_connection::accept_connection()"); }
      } accept_connection_scope_logger;

      try
      {
        assert( our_state == our_connection_state::disconnected &&
                their_state == their_connection_state::disconnected );
        direction = peer_connection_direction::inbound;
        negotiation_status = connection_negotiation_status::accepting;
        _message_connection.accept();           // perform key exchange
        negotiation_status = connection_negotiation_status::accepted;
        _remote_endpoint = _message_connection.get_socket().remote_endpoint();

        // firewall-detecting info is pretty useless for inbound connections, but initialize
        // it the best we can
        fc::ip::endpoint local_endpoint = _message_connection.get_socket().local_endpoint();
        inbound_address = local_endpoint.get_address();
        inbound_port = local_endpoint.port();
        outbound_port = inbound_port;

        their_state = their_connection_state::just_connected;
        our_state = our_connection_state::just_connected;
        ilog( "established inbound connection from ${remote_endpoint}, sending hello", ("remote_endpoint", _message_connection.get_socket().remote_endpoint() ) );
      }
      catch ( const fc::exception& e )
      {
        wlog( "error accepting connection ${e}", ("e", e.to_detail_string() ) );
        throw;
      }
    }

    void peer_connection::connect_to( const fc::ip::endpoint& remote_endpoint, fc::optional<fc::ip::endpoint> local_endpoint )
    {
      VERIFY_CORRECT_THREAD();
      try
      {
        assert( our_state == our_connection_state::disconnected &&
                their_state == their_connection_state::disconnected );
        direction = peer_connection_direction::outbound;

        _remote_endpoint = remote_endpoint;
        if( local_endpoint )
        {
          // the caller wants us to bind the local side of this socket to a specific ip/port
          // This depends on the ip/port being unused, and on being able to set the
          // SO_REUSEADDR/SO_REUSEPORT flags, and either of these might fail, so we need to
          // detect if this fails.
          try
          {
            _message_connection.bind( *local_endpoint );
          }
          catch ( const fc::canceled_exception& )
          {
            throw;
          }
          catch ( const fc::exception& except )
          {
            wlog( "Failed to bind to desired local endpoint ${endpoint}, will connect using an OS-selected endpoint: ${except}", ("endpoint", *local_endpoint )("except", except ) );
          }
        }
        negotiation_status = connection_negotiation_status::connecting;
        _message_connection.connect_to( remote_endpoint );
        negotiation_status = connection_negotiation_status::connected;
        their_state = their_connection_state::just_connected;
        our_state = our_connection_state::just_connected;
        ilog( "established outbound connection to ${remote_endpoint}", ("remote_endpoint", remote_endpoint ) );
      }
      catch ( fc::exception& e )
      {
        elog( "fatal: error connecting to peer ${remote_endpoint}: ${e}", ("remote_endpoint", remote_endpoint )("e", e.to_detail_string() ) );
        throw;
      }
    } // connect_to()

    void peer_connection::on_message( message_oriented_connection* originating_connection, const message& received_message )
    {
      VERIFY_CORRECT_THREAD();
      _node->on_message( this, received_message );
    }

    void peer_connection::on_connection_closed( message_oriented_connection* originating_connection )
    {
      VERIFY_CORRECT_THREAD();
      negotiation_status = connection_negotiation_status::closed;
      _node->on_connection_closed( this );
    }

    void peer_connection::send_queued_messages_task()
    {
      VERIFY_CORRECT_THREAD();
#ifndef NDEBUG
      struct counter {
        unsigned& _send_message_queue_tasks_counter;
        counter(unsigned& var) : _send_message_queue_tasks_counter(var) { /* dlog("entering peer_connection::send_queued_messages_task()"); */ assert(_send_message_queue_tasks_counter == 0); ++_send_message_queue_tasks_counter; }
        ~counter() { assert(_send_message_queue_tasks_counter == 1); --_send_message_queue_tasks_counter; /* dlog("leaving peer_connection::send_queued_messages_task()"); */ }
      } concurrent_invocation_counter(_send_message_queue_tasks_running);
#endif
      while (!_queued_messages.empty())
      {
        _queued_messages.front()->transmission_start_time = fc::time_point::now();
        message message_to_send = _queued_messages.front()->get_message(_node);
        try
        {
          //dlog("peer_connection::send_queued_messages_task() calling message_oriented_connection::send_message() "
          //     "to send message of type ${type} for peer ${endpoint}",
          //     ("type", message_to_send.msg_type)("endpoint", get_remote_endpoint()));
          _message_connection.send_message(message_to_send);
          //dlog("peer_connection::send_queued_messages_task()'s call to message_oriented_connection::send_message() completed normally for peer ${endpoint}",
          //     ("endpoint", get_remote_endpoint()));
        }
        catch (const fc::canceled_exception&)
        {
          dlog("message_oriented_connection::send_message() was canceled, rethrowing canceled_exception");
          throw;
        }
        catch (const fc::exception& send_error)
        {
          elog("Error sending message: ${exception}.  Closing connection.", ("exception", send_error));
          try
          {
            close_connection();
          }
          catch (const fc::exception& close_error)
          {
            elog("Caught error while closing connection: ${exception}", ("exception", close_error));
          }
          return;
        }
        catch (const std::exception& e)
        {
          elog("message_oriented_exception::send_message() threw a std::exception(): ${what}", ("what", e.what()));
        }
        catch (...)
        {
          elog("message_oriented_exception::send_message() threw an unhandled exception");
        }
        _queued_messages.front()->transmission_finish_time = fc::time_point::now();
        _total_queued_messages_size -= _queued_messages.front()->get_size_in_queue();
        _queued_messages.pop();
      }
      //dlog("leaving peer_connection::send_queued_messages_task() due to queue exhaustion");
    }

    void peer_connection::send_queueable_message(std::unique_ptr<queued_message>&& message_to_send)
    {
      VERIFY_CORRECT_THREAD();
      _total_queued_messages_size += message_to_send->get_size_in_queue();
      _queued_messages.emplace(std::move(message_to_send));
      if (_total_queued_messages_size > EOS_NET_MAXIMUM_QUEUED_MESSAGES_IN_BYTES)
      {
        elog("send queue exceeded maximum size of ${max} bytes (current size ${current} bytes)",
             ("max", EOS_NET_MAXIMUM_QUEUED_MESSAGES_IN_BYTES)("current", _total_queued_messages_size));
        try
        {
          close_connection();
        }
        catch (const fc::exception& e)
        {
          elog("Caught error while closing connection: ${exception}", ("exception", e));
        }
        return;
      }

      if( _send_queued_messages_done.valid() && _send_queued_messages_done.canceled() )
        FC_THROW_EXCEPTION(fc::exception, "Attempting to send a message on a connection that is being shut down");

      if (!_send_queued_messages_done.valid() || _send_queued_messages_done.ready())
      {
        //dlog("peer_connection::send_message() is firing up send_queued_message_task");
        _send_queued_messages_done = fc::async([this](){ send_queued_messages_task(); }, "send_queued_messages_task");
      }
      //else
      //  dlog("peer_connection::send_message() doesn't need to fire up send_queued_message_task, it's already running");
    }

    void peer_connection::send_message(const message& message_to_send, size_t message_send_time_field_offset)
    {
      VERIFY_CORRECT_THREAD();
      //dlog("peer_connection::send_message() enqueueing message of type ${type} for peer ${endpoint}",
      //     ("type", message_to_send.msg_type)("endpoint", get_remote_endpoint()));
      std::unique_ptr<queued_message> message_to_enqueue(new real_queued_message(message_to_send, message_send_time_field_offset));
      send_queueable_message(std::move(message_to_enqueue));
    }

    void peer_connection::send_item(const item_id& item_to_send)
    {
      VERIFY_CORRECT_THREAD();
      //dlog("peer_connection::send_item() enqueueing message of type ${type} for peer ${endpoint}",
      //     ("type", item_to_send.item_type)("endpoint", get_remote_endpoint()));
      std::unique_ptr<queued_message> message_to_enqueue(new virtual_queued_message(item_to_send));
      send_queueable_message(std::move(message_to_enqueue));
    }

    void peer_connection::close_connection()
    {
      VERIFY_CORRECT_THREAD();
      negotiation_status = connection_negotiation_status::closing;
      if (connection_terminated_time != fc::time_point::min())
        connection_terminated_time = fc::time_point::now();
      _message_connection.close_connection();
    }

    void peer_connection::destroy_connection()
    {
      VERIFY_CORRECT_THREAD();
      negotiation_status = connection_negotiation_status::closing;
      destroy();
    }

    uint64_t peer_connection::get_total_bytes_sent() const
    {
      VERIFY_CORRECT_THREAD();
      return _message_connection.get_total_bytes_sent();
    }

    uint64_t peer_connection::get_total_bytes_received() const
    {
      VERIFY_CORRECT_THREAD();
      return _message_connection.get_total_bytes_received();
    }

    fc::time_point peer_connection::get_last_message_sent_time() const
    {
      VERIFY_CORRECT_THREAD();
      return _message_connection.get_last_message_sent_time();
    }

    fc::time_point peer_connection::get_last_message_received_time() const
    {
      VERIFY_CORRECT_THREAD();
      return _message_connection.get_last_message_received_time();
    }

    fc::optional<fc::ip::endpoint> peer_connection::get_remote_endpoint()
    {
      VERIFY_CORRECT_THREAD();
      return _remote_endpoint;
    }
    fc::ip::endpoint peer_connection::get_local_endpoint()
    {
      VERIFY_CORRECT_THREAD();
      return _message_connection.get_socket().local_endpoint();
    }

    void peer_connection::set_remote_endpoint( fc::optional<fc::ip::endpoint> new_remote_endpoint )
    {
      VERIFY_CORRECT_THREAD();
      _remote_endpoint = new_remote_endpoint;
    }

    bool peer_connection::busy()
    {
      VERIFY_CORRECT_THREAD();
      return !items_requested_from_peer.empty() || !sync_items_requested_from_peer.empty() || item_ids_requested_from_peer;
    }

    bool peer_connection::idle()
    {
      VERIFY_CORRECT_THREAD();
      return !busy();
    }

    bool peer_connection::is_transaction_fetching_inhibited() const
    {
      VERIFY_CORRECT_THREAD();
      return transaction_fetching_inhibited_until > fc::time_point::now();
    }

    fc::sha512 peer_connection::get_shared_secret() const
    {
      VERIFY_CORRECT_THREAD();
      return _message_connection.get_shared_secret();
    }

    void peer_connection::clear_old_inventory()
    {
      VERIFY_CORRECT_THREAD();
      fc::time_point_sec oldest_inventory_to_keep(fc::time_point::now() - fc::minutes(EOS_NET_MAX_INVENTORY_SIZE_IN_MINUTES));

      // expire old items from inventory_advertised_to_peer
      auto oldest_inventory_to_keep_iter = inventory_advertised_to_peer.get<timestamp_index>().lower_bound(oldest_inventory_to_keep);
      auto begin_iter = inventory_advertised_to_peer.get<timestamp_index>().begin();
      unsigned number_of_elements_advertised_to_peer_to_discard = std::distance(begin_iter, oldest_inventory_to_keep_iter);
      inventory_advertised_to_peer.get<timestamp_index>().erase(begin_iter, oldest_inventory_to_keep_iter);

      // also expire items from inventory_peer_advertised_to_us
      oldest_inventory_to_keep_iter = inventory_peer_advertised_to_us.get<timestamp_index>().lower_bound(oldest_inventory_to_keep);
      begin_iter = inventory_peer_advertised_to_us.get<timestamp_index>().begin();
      unsigned number_of_elements_peer_advertised_to_discard = std::distance(begin_iter, oldest_inventory_to_keep_iter);
      inventory_peer_advertised_to_us.get<timestamp_index>().erase(begin_iter, oldest_inventory_to_keep_iter);
      dlog("Expiring old inventory for peer ${peer}: removing ${to_peer} items advertised to peer (${remain_to_peer} left), and ${to_us} advertised to us (${remain_to_us} left)",
           ("peer", get_remote_endpoint())
           ("to_peer", number_of_elements_advertised_to_peer_to_discard)("remain_to_peer", inventory_advertised_to_peer.size())
           ("to_us", number_of_elements_peer_advertised_to_discard)("remain_to_us", inventory_peer_advertised_to_us.size()));
    }

    // we have a higher limit for blocks than transactions so we will still fetch blocks even when transactions are throttled
    bool peer_connection::is_inventory_advertised_to_us_list_full_for_transactions() const
    {
      VERIFY_CORRECT_THREAD();
      return inventory_peer_advertised_to_us.size() > EOS_NET_MAX_INVENTORY_SIZE_IN_MINUTES * EOS_NET_MAX_TRX_PER_SECOND * 60;
    }

    bool peer_connection::is_inventory_advertised_to_us_list_full() const
    {
      VERIFY_CORRECT_THREAD();
      // allow the total inventory size to be the maximum number of transactions we'll store in the inventory (above)
      // plus the maximum number of blocks that would be generated in EOS_NET_MAX_INVENTORY_SIZE_IN_MINUTES (plus one,
      // to give us some wiggle room)
      return inventory_peer_advertised_to_us.size() >
        EOS_NET_MAX_INVENTORY_SIZE_IN_MINUTES * EOS_NET_MAX_TRX_PER_SECOND * 60 +
        (EOS_NET_MAX_INVENTORY_SIZE_IN_MINUTES + 1) * 60 / EOS_MIN_BLOCK_INTERVAL;
    }

    bool peer_connection::performing_firewall_check() const
    {
      return firewall_check_state && firewall_check_state->requesting_peer != node_id_t();
    }

    fc::optional<fc::ip::endpoint> peer_connection::get_endpoint_for_connecting() const
    {
      if (inbound_port)
        return fc::ip::endpoint(inbound_address, inbound_port);
      return fc::optional<fc::ip::endpoint>();
    }

} } // end namespace eos::net
