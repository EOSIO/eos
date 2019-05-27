#include <hidapi.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <cbor.h>
#include <stdexcept>
#include <vector>
#include <array>

#include <fc/crypto/public_key.hpp>

#include <microfido/microfido.hpp>

namespace eosio {

using namespace fc::crypto::webauthn;

const uint16_t fido_usage_page = 0xF1D0;
const uint16_t fido_u2f_usage = 0x0001;

struct __attribute__((__packed__)) u2f_init_frame {
   uint32_t cid;
   uint8_t cmd;
   uint16_t bcnt;
   uint8_t data[64-7];
};

struct __attribute__((__packed__)) u2f_cont_frame {
   uint32_t cid;
   uint8_t seq;
   uint8_t data[64-5];
};

struct __attribute__((__packed__)) init_resp_msg {
   uint64_t nonce;
   uint32_t channel_id;
   uint8_t protocol_version;
   uint8_t major_version;
   uint8_t minor_version;
   uint8_t build_version;
   uint8_t cap_flags;
};

struct __attribute__((__packed__)) create_auth_data {
   uint8_t rp_id_hash[32];
   uint8_t flags;
   uint32_t counter;
   uint8_t aaguid[16];
   uint16_t credential_id_length;
};

struct fido_device_impl {
   hid_device* _dev = nullptr;
   uint32_t _channel_id = 0xffffffff;

   std::vector<uint8_t> send_recv(uint8_t cmd, std::vector<uint8_t> send) {
      uint8_t sendbuff[sizeof(u2f_init_frame)+1] = {}; //extra byte for the hid report num
      u2f_init_frame* init_frame = reinterpret_cast<u2f_init_frame*>(sendbuff+1);
      u2f_cont_frame* cont_frame = reinterpret_cast<u2f_cont_frame*>(sendbuff+1);
      size_t sent = 0;
      size_t c = 0;

      init_frame->cid = _channel_id;
      init_frame->cmd = cmd | 0x80;
      init_frame->bcnt = htons(send.size());
      sent = std::min(send.size(), sizeof(init_frame->data));
      memcpy(init_frame->data, send.data(), sent);
      int wrot = hid_write(_dev, sendbuff, sizeof(sendbuff));

      cont_frame->cid = _channel_id;
      cont_frame->seq = 0;
      while(sent < send.size()) {
         memset(cont_frame->data, 0, sizeof(cont_frame->data));
         size_t to_send = std::min(send.size() - sent, sizeof(cont_frame->data));
         memcpy(cont_frame->data, send.data()+sent, to_send);
         int wrot = hid_write(_dev, sendbuff, sizeof(sendbuff));
         sent += to_send;
         cont_frame->seq++;
      }

      std::vector<uint8_t> resp;
      bool found_reply = false;
      do {
         resp.clear();
         hid_read(_dev, (uint8_t*)init_frame, sizeof(*init_frame));
         if(init_frame->cid != _channel_id)
            continue;
         int left = ntohs(init_frame->bcnt);
         size_t got_this_frame = std::min(left, (int)sizeof(init_frame->data));
         resp.insert(resp.end(), init_frame->data, init_frame->data+got_this_frame);
         left -= got_this_frame;
         if(init_frame->cmd == (0x3f | 0x80))
            throw std::runtime_error("eh");
         found_reply = init_frame->cmd != (0x3b | 0x80); ///XXX not nearly as robust as it should be

         while(left > 0) {
            hid_read(_dev, (uint8_t*)cont_frame, sizeof(*cont_frame));
            ///XXX check sequence
            if(cont_frame->cid != _channel_id)
               continue;
            got_this_frame = std::min(left, (int)sizeof(cont_frame->data));
            resp.insert(resp.end(), cont_frame->data, cont_frame->data+got_this_frame);
            left -= got_this_frame;
         }
      } while(!found_reply);

      return resp;
   }
};

fido_device::fido_device() : my(new fido_device_impl()) {
   struct hid_device_info* di = hid_enumerate(0, 0);

   for(struct hid_device_info* d = di; d; d = d->next) {
      if(d->usage_page == fido_usage_page && d->usage == fido_u2f_usage) {
         my->_dev = hid_open_path(d->path);
         break;
      }
   }

   hid_free_enumeration(di);
   FC_ASSERT(my->_dev, "No FIDO2 device found (try again, it can be buggy)");

   std::vector<uint8_t> init_nonce(8); //XXX random
   std::vector<uint8_t> init_resp_data = my->send_recv(0x06, init_nonce);
   ///XXX check size, check nonce
   init_resp_msg* init_resp = reinterpret_cast<init_resp_msg*>(init_resp_data.data());

   my->_channel_id =  init_resp->channel_id;
}

fido_device::~fido_device() {
   if(my->_dev)
      hid_close(my->_dev);
}

fido_registration_result fido_device::do_registration(std::array<uint8_t, 32> client_data_hash) {
   std::vector<uint8_t> sendbuff(8192);
   sendbuff[0] = 0x01; //make credential

   CborEncoder encoder, d1_encoder, d2_encoder, d3_encoder;
   cbor_encoder_init(&encoder, sendbuff.data()+1, sendbuff.size()-1, 0);
   cbor_encoder_create_map(&encoder, &d1_encoder, 4);
      //encode client data hash
      cbor_encode_uint(&d1_encoder, 1U);
      cbor_encode_byte_string(&d1_encoder, client_data_hash.data(), client_data_hash.size());

      //encode RP map
      cbor_encode_uint(&d1_encoder, 2U);
      cbor_encoder_create_map(&d1_encoder, &d2_encoder, 1);
         cbor_encode_text_stringz(&d2_encoder, "id");
         cbor_encode_text_stringz(&d2_encoder, "keosd.invalid");
      cbor_encoder_close_container(&d1_encoder, &d2_encoder);

      //encode user map
      cbor_encode_uint(&d1_encoder, 3U);
      cbor_encoder_create_map(&d1_encoder, &d2_encoder, 2);
         cbor_encode_text_stringz(&d2_encoder, "id");
         uint8_t blah[32];
         cbor_encode_byte_string(&d2_encoder, blah, sizeof(blah));
         //cbor_encode_text_stringz(&d2_encoder, "keosd created key");
         cbor_encode_text_stringz(&d2_encoder, "name");
         cbor_encode_text_stringz(&d2_encoder, "keosd created key");
      cbor_encoder_close_container(&d1_encoder, &d2_encoder);

      //encode pubKeyCredParams
      cbor_encode_uint(&d1_encoder, 4U);
      cbor_encoder_create_array(&d1_encoder, &d2_encoder, 1);
         cbor_encoder_create_map(&d2_encoder, &d3_encoder, 2);
            cbor_encode_text_stringz(&d3_encoder, "alg");
            cbor_encode_int(&d3_encoder, -7);
            cbor_encode_text_stringz(&d3_encoder, "type");
            cbor_encode_text_stringz(&d3_encoder, "public-key");
         cbor_encoder_close_container(&d2_encoder, &d3_encoder);
      cbor_encoder_close_container(&d1_encoder, &d2_encoder);
   cbor_encoder_close_container(&encoder, &d1_encoder);
   size_t cbor_size = cbor_encoder_get_buffer_size(&encoder, sendbuff.data()+1);
   sendbuff.resize(cbor_size + 1);
   
   std::vector<uint8_t> resp = my->send_recv(0x10, sendbuff);

   ///XXX check return code of command
   CborParser parser;
   CborValue it;
   cbor_parser_init(resp.data()+1, resp.size()-1, 0, &parser, &it);
   //expect just a map
   cbor_value_enter_container(&it, &it);
   
   while(!cbor_value_at_end(&it)) {
      int found_key = 0;
      cbor_value_get_int_checked(&it, &found_key);
      cbor_value_advance(&it);
      if(found_key == 2)
         break;
      cbor_value_advance(&it);
   }
   if(cbor_value_at_end(&it)) {} //XXX throw something
   size_t auth_data_size;
   cbor_value_get_string_length(&it, &auth_data_size);
   std::vector<uint8_t> auth_data(auth_data_size);
   cbor_value_copy_byte_string(&it, auth_data.data(), &auth_data_size, &it);

   //XX check sizes of all this junk
   create_auth_data* auth_data_reply = reinterpret_cast<create_auth_data*>(auth_data.data());
   uint16_t credential_id_length_host = ntohs(auth_data_reply->credential_id_length);
   std::vector<uint8_t> credential_id(auth_data.data()+sizeof(create_auth_data), auth_data.data()+sizeof(create_auth_data)+credential_id_length_host);
   
   cbor_parser_init(auth_data.data()+sizeof(create_auth_data)+credential_id_length_host, auth_data.size()-credential_id_length_host-sizeof(create_auth_data), 0, &parser, &it);
   std::vector<uint8_t> pub_x(32), pub_y(32);
   //expect just a map
   cbor_value_enter_container(&it, &it);
   //XXX should check stuff better here by a lot!
   while(!cbor_value_at_end(&it)) {
      int found_key = 0;
      cbor_value_get_int_checked(&it, &found_key);
      cbor_value_advance(&it);
      size_t bytes_size = 32;
      if(found_key == -2)
         cbor_value_copy_byte_string(&it, pub_x.data(), &bytes_size, &it);
      else if(found_key == -3)
         cbor_value_copy_byte_string(&it, pub_y.data(), &bytes_size, &it);
      else
         cbor_value_advance(&it);
   }

   public_key::public_key_data_type pub_key_data;
   pub_key_data.data[0] = 0x02 + (pub_y[31]&1); //even or odd Y
   memcpy(pub_key_data.data+1, pub_x.data(), 32); //copy in the 32 bytes of X
   public_key pub_key(pub_key_data, public_key::user_presence_t::USER_PRESENCE_PRESENT, "keosd.invalid");

   //fc::crypto::public_key still doesn't allow creation of itself with an instance of a storage_type
   fc::datastream<size_t> dsz;
   fc::raw::pack(dsz, pub_key);
   char packed_pub_key[dsz.tellp()+1];
   fc::datastream<char*> ds(packed_pub_key, sizeof(packed_pub_key));
   fc::raw::pack(ds, '\x02'); //webauthn type
   fc::raw::pack(ds, pub_key);
   ds.seekp(0);
   fc::crypto::public_key pk;
   fc::raw::unpack(ds, pk);

   return {
      .pub_key = pk,
      .credential_id = credential_id
   };
}

fido_assertion_result fido_device::do_assertion(const fc::sha256& client_data_hash, const std::vector<uint8_t>& credential_id) {
   std::vector<uint8_t> sendbuff(8192);
   sendbuff[0] = 0x02; //get assertion

   CborEncoder encoder, map_encoder, array_encoder, cred_map_encoder;
   cbor_encoder_init(&encoder, sendbuff.data()+1, sendbuff.size()-1, 0);
   cbor_encoder_create_map(&encoder, &map_encoder, 3);
      //RP
      cbor_encode_uint(&map_encoder, 1U);
      cbor_encode_text_stringz(&map_encoder, "keosd.invalid");

      //client data hash
      cbor_encode_uint(&map_encoder, 2U);
      cbor_encode_byte_string(&map_encoder, (uint8_t*)client_data_hash.data(), client_data_hash.data_size());

      //credential ID
      cbor_encode_uint(&map_encoder, 3U);
      cbor_encoder_create_array(&map_encoder, &array_encoder, 1);
         cbor_encoder_create_map(&array_encoder, &cred_map_encoder, 2);
            cbor_encode_text_stringz(&cred_map_encoder, "id");
            cbor_encode_byte_string(&cred_map_encoder, credential_id.data(), credential_id.size());

            cbor_encode_text_stringz(&cred_map_encoder, "type");
            cbor_encode_text_stringz(&cred_map_encoder, "public-key");
         cbor_encoder_close_container(&array_encoder, &cred_map_encoder);
      cbor_encoder_close_container(&map_encoder, &array_encoder);
   cbor_encoder_close_container(&encoder, &map_encoder);
   size_t cbor_size = cbor_encoder_get_buffer_size(&encoder, sendbuff.data()+1);

   sendbuff.resize(cbor_size + 1);
   std::vector<uint8_t> resp = my->send_recv(0x10, sendbuff);

   std::vector<uint8_t> auth_data(256);
   size_t auth_data_size = auth_data.size();
   std::vector<uint8_t> der_sig(256);
   size_t der_sig_size = der_sig.size();

   CborParser parser;
   CborValue it;
   cbor_parser_init(resp.data()+1, resp.size()-1, 0, &parser, &it);
   //expect just a map
   cbor_value_enter_container(&it, &it);

   while(!cbor_value_at_end(&it)) {
      int found_key = 0;
      cbor_value_get_int_checked(&it, &found_key);
      cbor_value_advance(&it);
      if(found_key == 2)
         cbor_value_copy_byte_string(&it, auth_data.data(), &auth_data_size, &it);
      else if(found_key == 3) {
         cbor_value_copy_byte_string(&it, der_sig.data(), &der_sig_size, &it);
      }
      else
         cbor_value_advance(&it);
   }
   ///XXX throw if didn't get sig & auth
   auth_data.resize(auth_data_size);
   der_sig.resize(der_sig_size);

   return {
      .auth_data = auth_data,
      .der_signature = der_sig
   };
}

}