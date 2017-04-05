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
#include <eos/net/core_messages.hpp>


namespace eos { namespace net {

  const core_message_type_enum trx_message::type                             = core_message_type_enum::trx_message_type;
  const core_message_type_enum block_message::type                           = core_message_type_enum::block_message_type;
  const core_message_type_enum item_ids_inventory_message::type              = core_message_type_enum::item_ids_inventory_message_type;
  const core_message_type_enum blockchain_item_ids_inventory_message::type   = core_message_type_enum::blockchain_item_ids_inventory_message_type;
  const core_message_type_enum fetch_blockchain_item_ids_message::type       = core_message_type_enum::fetch_blockchain_item_ids_message_type;
  const core_message_type_enum fetch_items_message::type                     = core_message_type_enum::fetch_items_message_type;
  const core_message_type_enum item_not_available_message::type              = core_message_type_enum::item_not_available_message_type;
  const core_message_type_enum hello_message::type                           = core_message_type_enum::hello_message_type;
  const core_message_type_enum connection_accepted_message::type             = core_message_type_enum::connection_accepted_message_type;
  const core_message_type_enum connection_rejected_message::type             = core_message_type_enum::connection_rejected_message_type;
  const core_message_type_enum address_request_message::type                 = core_message_type_enum::address_request_message_type;
  const core_message_type_enum address_message::type                         = core_message_type_enum::address_message_type;
  const core_message_type_enum closing_connection_message::type              = core_message_type_enum::closing_connection_message_type;
  const core_message_type_enum current_time_request_message::type            = core_message_type_enum::current_time_request_message_type;
  const core_message_type_enum current_time_reply_message::type              = core_message_type_enum::current_time_reply_message_type;
  const core_message_type_enum check_firewall_message::type                  = core_message_type_enum::check_firewall_message_type;
  const core_message_type_enum check_firewall_reply_message::type            = core_message_type_enum::check_firewall_reply_message_type;
  const core_message_type_enum get_current_connections_request_message::type = core_message_type_enum::get_current_connections_request_message_type;
  const core_message_type_enum get_current_connections_reply_message::type   = core_message_type_enum::get_current_connections_reply_message_type;

} } // eos::net

