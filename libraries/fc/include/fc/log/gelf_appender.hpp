#pragma once

#include <fc/log/appender.hpp>
#include <fc/log/logger.hpp>
#include <fc/time.hpp>

namespace fc 
{
  // Log appender that sends log messages in JSON format over UDP
  // https://www.graylog2.org/resources/gelf/specification
  class gelf_appender : public appender 
  {
  public:
    struct config 
    {
      string endpoint = "127.0.0.1:12201";
      string host = "fc"; // the name of the host, source or application that sent this message (just passed through to GELF server)
    };

    gelf_appender(const variant& args);
    ~gelf_appender();
    /** \brief Required for name resolution and socket initialization.
     *
     * \warning If this method is not called, this appender will log nothing.
     *
     * In a single-threaded world with a boost::io_service that's not owned
     * by this library, ugly things are required.  Tough.
     */
    void initialize(boost::asio::io_service& io_service) override;
    virtual void log(const log_message& m) override;

  private:
    class impl;
    fc::shared_ptr<impl> my;
  };
} // namespace fc

#include <fc/reflect/reflect.hpp>
FC_REFLECT(fc::gelf_appender::config,
           (endpoint)(host))
