#include <fc/network/gntp.hpp>
#include <fc/exception/exception.hpp>

#include <fc/log/logger.hpp>
#include <fc/asio.hpp>
#include <fc/network/tcp_socket.hpp>
#include <fc/crypto/sha1.hpp>
#include <fc/crypto/base32.hpp>
#include <fc/crypto/sha256.hpp>
#include <fc/crypto/rand.hpp>
#include <fc/crypto/hex.hpp>

#include <set>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string/case_conv.hpp>

namespace fc 
{
  namespace detail
  {
    static std::string calc_sha1_base32_of_buffer(const std::string& buffer)
    {
      sha1::encoder sha1_encoder;
      sha1_encoder.write(buffer.c_str(), buffer.size());
      sha1 sha1_result = sha1_encoder.result();
      string sha1_result_base32 = to_base32((char*)&sha1_result, sizeof(sha1_result));
      return sha1_result_base32.c_str();
    }


    class gntp_icon_impl
    {
    public:
      std::string _icon_bytes;
      std::string _sha1_hash;

      gntp_icon_impl(const char* buffer, size_t length) :
        _icon_bytes(buffer, length),
        _sha1_hash(calc_sha1_base32_of_buffer(_icon_bytes))
      {
      }
    };

    class gntp_notifier_impl 
    {
    public:
      gntp_notifier_impl(const std::string& host_to_notify = "127.0.0.1", uint16_t port = 23053, const optional<std::string>& password = optional<std::string>());

      // there's no API to change these right now, it will always notify localhost at the default GNTP port
      std::string hostname;
      uint16_t port;
      optional<std::string> password;

      std::string application_name;
      gntp_icon_ptr application_icon;

      gntp_notification_type_list notification_types; // list of all notification types we're registered to send

      optional<boost::asio::ip::tcp::endpoint> endpoint; // cache the last endpoint we've connected to

      bool connection_failed; // true after we've tried to connect and failed
      bool is_registered; // true after we've registered

      void send_gntp_message(const std::string& message);
    };

    gntp_notifier_impl::gntp_notifier_impl(const std::string& host_to_notify /* = "127.0.0.1" */, uint16_t port /* = 23053 */,
                                           const optional<std::string>& password /* = optional<std::string>() */) :
      hostname(host_to_notify),
      port(port),
      password(password),
      connection_failed(false),
      is_registered(false)
    {
    }

    void gntp_notifier_impl::send_gntp_message(const std::string& message)
    {
      std::shared_ptr<boost::asio::ip::tcp::socket> sock(new boost::asio::ip::tcp::socket(asio::default_io_service()));

      bool connected = false;
      if (endpoint)
      {
        // we've successfully connected before, connect to the same endpoint that worked last time
        try
        {
          asio::tcp::connect(*sock, *endpoint);
          connected = true;
        }
        catch (exception& er)
        {
          ilog("Failed to connect to GNTP service using an endpoint that previously worked: ${error_report}", 
              ("error_report", er.to_detail_string()));
          sock->close();
          // clear the cached endpoint and fall through to the full connection procedure
          endpoint = optional<boost::asio::ip::tcp::endpoint>();
        }
        catch (...)
        {
          ilog("Failed to connect to GNTP service using an endpoint that previously worked");
          sock->close();
          // clear the cached endpoint and fall through to the full connection procedure
          endpoint = optional<boost::asio::ip::tcp::endpoint>();
        }
      }
      if (!connected)
      {
        // do the full connection procedure
        auto eps = asio::tcp::resolve(hostname, boost::lexical_cast<std::string>(port));
        if (eps.size() == 0)
          FC_THROW("Unable to resolve host '${host}'", ("host", hostname));

        for (uint32_t i = 0; i < eps.size(); ++i)
        {
          try 
          {
            boost::system::error_code ec;
            ilog("Attempting to connect to GNTP srvice");
            asio::tcp::connect(*sock, eps[i]);
            endpoint = eps[i];
            connected = true;
            break;
          }
          catch (const exception& er) 
          {
            ilog("Failed to connect to GNTP service: ${error_reprot}", 
                  ("error_report", er.to_detail_string()) );
            sock->close();
          }
          catch (...)
          {
            ilog("Failed to connect to GNTP service");
            sock->close();
          }
        }
      }
      if (!connected)
        FC_THROW("Unable to connect to any resolved endpoint for ${host}:${port}", 
                  ("host", hostname)("port", port));
      try
      {
        asio::ostream<boost::asio::ip::tcp::socket> write_stream(sock);
        write_stream.write(message.c_str(), message.size());
        write_stream.flush();
        write_stream.close();
      }
      catch (exception& er)
      {
        FC_RETHROW_EXCEPTION(er, warn, "Caught an exception while sending data to GNTP service");
      }
      catch (...)
      {
        FC_THROW("Caught an exception while sending data to GNTP service");
      }
    }
  } // end namespace detail

  gntp_icon::gntp_icon(const char* buffer, size_t length) :
    my(new detail::gntp_icon_impl(buffer, length))
  {
  }
  gntp_icon::~gntp_icon()
  {
  }
   
  gntp_notifier::gntp_notifier(const std::string& host_to_notify /* = "127.0.0.1" */, uint16_t port /* = 23053 */,
                               const optional<std::string>& password /* = optional<std::string>() */) :
    my(new detail::gntp_notifier_impl(host_to_notify, port, password))
  {
  }

  gntp_notifier::~gntp_notifier()
  {
  }

  void gntp_notifier::set_application_name(std::string appName)
  {
    my->application_name = appName;
  }
  void gntp_notifier::set_application_icon(const gntp_icon_ptr& icon)
  {
    my->application_icon = icon;
  }
  void gntp_notifier::add_notification_type(const gntp_notification_type& notification_type)
  {
    my->notification_types.push_back(notification_type);
  }

  void gntp_notifier::register_notifications()
  {
    // this call will reset any errors
    my->connection_failed = false;
    my->is_registered = false;

    std::ostringstream message;
    std::set<gntp_icon_ptr> icons_used;

    message << "GNTP/1.0 REGISTER NONE\r\n";
    message << "Application-Name: " << my->application_name << "\r\n";
    if (my->application_icon)
    {
      message << "Application-Icon: x-growl-resource://" << my->application_icon->my->_sha1_hash << "\r\n";
      icons_used.insert(my->application_icon);
    }

    message << "Notifications-Count: " << my->notification_types.size() << "\r\n";
    for (const gntp_notification_type& notification_type : my->notification_types)
    {
      message << "\r\n";
      message << "Notification-Name: " << notification_type.name << "\r\n";
      if (!notification_type.display_name.empty())
        message << "Notification-Display-Name: " << notification_type.display_name << "\r\n";
      if (notification_type.icon)
      {
        message << "Notification-Icon: x-growl-resource://" << notification_type.icon->my->_sha1_hash << "\r\n";
        icons_used.insert(notification_type.icon);
      }
      message << "Notification-Enabled: " << (notification_type.enabled ? "True" : "False") << "\r\n";
    }
    if (!icons_used.empty())
    {
      message << "\r\n";
      for (const gntp_icon_ptr& icon : icons_used)
      {
        message << "Identifier: " << icon->my->_sha1_hash << "\r\n";
        message << "Length: " << icon->my->_icon_bytes.size() << "\r\n";
        message << "\r\n";
        message << icon->my->_icon_bytes;
        message << "\r\n";
      }
    }

    message << "\r\n\r\n";
    try
    {
      my->send_gntp_message(message.str());
      my->is_registered = true;
    }
    catch (const exception&)
    {
      my->connection_failed = true;
    }
  }
  gntp_guid gntp_notifier::send_notification(std::string name, std::string title, std::string text, 
                                             const gntp_icon_ptr& icon, optional<gntp_guid> coalescingId /* = optional<gntp_guid>() */)
  {
    if (my->connection_failed)
      return gntp_guid();
    if (!my->is_registered)
      return gntp_guid();

    gntp_guid notification_id;
    rand_pseudo_bytes(notification_id.data(), 20);

    std::ostringstream message;
    message << "GNTP/1.0 NOTIFY NONE";
    if (my->password)
    {
      char salt[16];
      rand_pseudo_bytes(salt, sizeof(salt));
      std::string salted_password = *my->password + std::string(salt, 16);
      sha256 key = sha256::hash(salted_password);
      sha256 keyhash = sha256::hash(key.data(), 32);
      message << " SHA256:" << boost::to_upper_copy(to_hex(keyhash.data(), 32)) << "." << boost::to_upper_copy(to_hex(salt, sizeof(salt)));
    }
    message << "\r\n";
    message << "Application-Name: " << my->application_name << "\r\n";
    message << "Notification-Name: " << name << "\r\n";
    message << "Notification-ID: " << notification_id.str() << "\r\n";
    message << "Notification-Coalescing-ID: " << (coalescingId ? coalescingId->str() : notification_id.str()) << "\r\n";
    message << "Notification-Title: " << title << "\r\n";
    message << "Notification-Text: " << text << "\r\n";
    if (icon)
      message << "Notification-Icon: x-growl-resource://" << icon->my->_sha1_hash << "\r\n";
      
    if (icon)
    {
      message << "\r\n";
      message << "Identifier: " << icon->my->_sha1_hash << "\r\n";
      message << "Length: " << icon->my->_icon_bytes.size() << "\r\n";
      message << "\r\n";
      message << icon->my->_icon_bytes;
      message << "\r\n";
    }
    message << "\r\n\r\n";
    my->send_gntp_message(message.str());
    return notification_id;
  }

} // namespace fc