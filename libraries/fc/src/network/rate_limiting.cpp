#include <fc/network/rate_limiting.hpp>
#include <fc/network/tcp_socket_io_hooks.hpp>
#include <fc/network/tcp_socket.hpp>
#include <list>
#include <algorithm>
#include <fc/network/ip.hpp>
#include <fc/fwd_impl.hpp>
#include <fc/asio.hpp>
#include <fc/log/logger.hpp>
#include <fc/io/stdio.hpp>
#include <fc/exception/exception.hpp>
#include <fc/thread/thread.hpp>

namespace fc
{

  namespace detail
  {
    // data about a read or write we're managing
    class rate_limited_operation
    {
    public:
      size_t                        length;
      size_t                        offset;
      size_t                        permitted_length;
      promise<size_t>::ptr          completion_promise;

      rate_limited_operation(size_t length,
                             size_t offset,
                             promise<size_t>::ptr&& completion_promise) :
        length(length),
        offset(offset),
        permitted_length(0),
        completion_promise(completion_promise)
      {}

      virtual void perform_operation() = 0;
    };

    class rate_limited_tcp_write_operation : public rate_limited_operation
    {
    public:
      boost::asio::ip::tcp::socket& socket;
      const char*                   raw_buffer;
      std::shared_ptr<const char>   shared_buffer;

      // QUESTION: Why would this version of the constructor ever be called if it will abort the program if built as DEBUG?
      //           Commenting out for now since this file is not even included in the fc library build.
      /*
      rate_limited_tcp_write_operation(boost::asio::ip::tcp::socket& socket,
                                       const char* buffer,
                                       size_t length,
                                       size_t offset,
                                       promise<size_t>::ptr completion_promise) :
        rate_limited_operation(length, offset, std::move(completion_promise)),
        socket(socket),
        raw_buffer(buffer)
      {
        assert(false);
      }
      */
      rate_limited_tcp_write_operation(boost::asio::ip::tcp::socket& socket,
                                       const std::shared_ptr<const char>& buffer,
                                       size_t length,
                                       size_t offset,
                                       promise<size_t>::ptr completion_promise) :
        rate_limited_operation(length, offset, std::move(completion_promise)),
        socket(socket),
        raw_buffer(nullptr),
        shared_buffer(buffer)
      {}
      virtual void perform_operation() override
      {
        if (raw_buffer)
          asio::async_write_some(socket,
                                 raw_buffer, permitted_length,
                                 completion_promise);
        else
          asio::async_write_some(socket,
                                 shared_buffer, permitted_length, offset,
                                 completion_promise);
      }
    };

    class rate_limited_tcp_read_operation : public rate_limited_operation
    {
    public:
      boost::asio::ip::tcp::socket& socket;
      char*                         raw_buffer;
      std::shared_ptr<char>         shared_buffer;

      rate_limited_tcp_read_operation(boost::asio::ip::tcp::socket& socket,
                                      char* buffer,
                                      size_t length,
                                      size_t offset,
                                      promise<size_t>::ptr completion_promise) :
        rate_limited_operation(length, offset, std::move(completion_promise)),
        socket(socket),
        raw_buffer(buffer)
      {}
      rate_limited_tcp_read_operation(boost::asio::ip::tcp::socket& socket,
                                      const std::shared_ptr<char>& buffer,
                                      size_t length,
                                      size_t offset,
                                      promise<size_t>::ptr completion_promise) :
        rate_limited_operation(length, offset, std::move(completion_promise)),
        socket(socket),
        raw_buffer(nullptr),
        shared_buffer(buffer)
      {}
      virtual void perform_operation() override
      {
        if (raw_buffer)
          asio::async_read_some(socket,
                                raw_buffer, permitted_length,
                                completion_promise);
        else
          asio::async_read_some(socket,
                                shared_buffer, permitted_length, offset,
                                completion_promise);

      }
    };

    struct is_operation_shorter
    {
      // less than operator designed to bring the shortest operations to the end
      bool operator()(const rate_limited_operation* lhs, const rate_limited_operation* rhs)
      {
        return lhs->length > rhs->length;
      }
    };

    class average_rate_meter
    {
    private:
      mutable double         _average_rate;
      mutable uint32_t       _unaccounted_bytes;
      mutable time_point     _last_update_time;
      microseconds           _time_constant;

      void update_const(uint32_t bytes_transferred = 0) const;
    public:
      average_rate_meter(const microseconds& time_constant = seconds(10));
      void set_time_constant(const microseconds& time_constant);
      void update(uint32_t bytes_transferred = 0);
      uint32_t get_average_rate() const;
    };
    average_rate_meter::average_rate_meter(const microseconds& time_constant) :
      _average_rate(0.),
      _unaccounted_bytes(0),
      _last_update_time(time_point_sec()),
      _time_constant(time_constant)
    {}
    void average_rate_meter::set_time_constant(const microseconds& time_constant)
    {
      _time_constant = time_constant;
    }
    void average_rate_meter::update(uint32_t bytes_transferred /* = 0 */)
    {
      update_const(bytes_transferred);
    }
    void average_rate_meter::update_const(uint32_t bytes_transferred /* = 0 */) const
    {
      time_point now = fc::now();
      if (now <= _last_update_time)
        _unaccounted_bytes += bytes_transferred;
      else
      {
        microseconds time_since_last_update = now - _last_update_time;
        if (time_since_last_update > _time_constant)
          _average_rate = bytes_transferred / (_time_constant.count() / (double)seconds(1).count());
        else
        {
          bytes_transferred += _unaccounted_bytes;
          double seconds_since_last_update = time_since_last_update.count() / (double)seconds(1).count();
          double rate_since_last_update = bytes_transferred / seconds_since_last_update;
          double alpha = time_since_last_update.count() / (double)_time_constant.count();
          _average_rate = rate_since_last_update * alpha + _average_rate * (1.0 - alpha);
        }
        _last_update_time = now;
        _unaccounted_bytes = 0;
      }
    }
    uint32_t average_rate_meter::get_average_rate() const
    {
      update_const();
      return (uint32_t)_average_rate;
    }

    class rate_limiting_group_impl : public tcp_socket_io_hooks
    {
    public:
      uint32_t _upload_bytes_per_second;
      uint32_t _download_bytes_per_second;
      uint32_t _burstiness_in_seconds;

      microseconds _granularity; // how often to add tokens to the bucket
      uint32_t _read_tokens;
      uint32_t _unused_read_tokens; // gets filled with tokens for unused bytes (if I'm allowed to read 200 bytes and I try to read 200 bytes, but can only read 50, tokens for the other 150 get returned here)
      uint32_t _write_tokens;
      uint32_t _unused_write_tokens;

      typedef std::list<rate_limited_operation*> rate_limited_operation_list;
      rate_limited_operation_list _read_operations_in_progress;
      rate_limited_operation_list _read_operations_for_next_iteration;
      rate_limited_operation_list _write_operations_in_progress;
      rate_limited_operation_list _write_operations_for_next_iteration;

      time_point _last_read_iteration_time;
      time_point _last_write_iteration_time;

      future<void> _process_pending_reads_loop_complete;
      promise<void>::ptr _new_read_operation_available_promise;
      future<void> _process_pending_writes_loop_complete;
      promise<void>::ptr _new_write_operation_available_promise;

      average_rate_meter _actual_upload_rate;
      average_rate_meter _actual_download_rate;

      rate_limiting_group_impl(uint32_t upload_bytes_per_second, uint32_t download_bytes_per_second,
                               uint32_t burstiness_in_seconds = 1);
      ~rate_limiting_group_impl();

      virtual size_t readsome(boost::asio::ip::tcp::socket& socket, char* buffer, size_t length) override;
      virtual size_t readsome(boost::asio::ip::tcp::socket& socket, const std::shared_ptr<char>& buffer, size_t length, size_t offset) override;
      template <typename BufferType>
      size_t readsome_impl(boost::asio::ip::tcp::socket& socket, const BufferType& buffer, size_t length, size_t offset);
      virtual size_t writesome(boost::asio::ip::tcp::socket& socket, const char* buffer, size_t length) override;
      virtual size_t writesome(boost::asio::ip::tcp::socket& socket, const std::shared_ptr<const char>& buffer, size_t length, size_t offset) override;
      template <typename BufferType>
      size_t writesome_impl(boost::asio::ip::tcp::socket& socket, const BufferType& buffer, size_t length, size_t offset);

      void process_pending_reads();
      void process_pending_writes();
      void process_pending_operations(time_point& last_iteration_start_time,
                                      uint32_t& limit_bytes_per_second,
                                      rate_limited_operation_list& operations_in_progress,
                                      rate_limited_operation_list& operations_for_next_iteration,
                                      uint32_t& tokens,
                                      uint32_t& unused_tokens);
    };

    rate_limiting_group_impl::rate_limiting_group_impl(uint32_t upload_bytes_per_second, uint32_t download_bytes_per_second,
                                                       uint32_t burstiness_in_seconds) :
      _upload_bytes_per_second(upload_bytes_per_second),
      _download_bytes_per_second(download_bytes_per_second),
      _burstiness_in_seconds(burstiness_in_seconds),
      _granularity(milliseconds(50)),
      _read_tokens(_download_bytes_per_second),
      _unused_read_tokens(0),
      _write_tokens(_upload_bytes_per_second),
      _unused_write_tokens(0)
    {
    }

    rate_limiting_group_impl::~rate_limiting_group_impl()
    {
      try
      {
        _process_pending_reads_loop_complete.cancel_and_wait();
      }
      catch (...)
      {
      }
      try
      {
        _process_pending_writes_loop_complete.cancel_and_wait();
      }
      catch (...)
      {
      }
    }

    size_t rate_limiting_group_impl::readsome(boost::asio::ip::tcp::socket& socket, const std::shared_ptr<char>& buffer, size_t length, size_t offset)
    {
      return readsome_impl(socket, buffer, length, offset);
    }

    size_t rate_limiting_group_impl::readsome(boost::asio::ip::tcp::socket& socket, char* buffer, size_t length)
    {
      return readsome_impl(socket, buffer, length, 0);
    }

    template <typename BufferType>
    size_t rate_limiting_group_impl::readsome_impl(boost::asio::ip::tcp::socket& socket, const BufferType& buffer, size_t length, size_t offset)
    {
      size_t bytes_read;
      if (_download_bytes_per_second)
      {
        promise<size_t>::ptr completion_promise(new promise<size_t>("rate_limiting_group_impl::readsome"));
        rate_limited_tcp_read_operation read_operation(socket, buffer, length, offset, completion_promise);
        _read_operations_for_next_iteration.push_back(&read_operation);

        // launch the read processing loop it if isn't running, or signal it to resume if it's paused.
        if (!_process_pending_reads_loop_complete.valid() || _process_pending_reads_loop_complete.ready())
          _process_pending_reads_loop_complete = async([=](){ process_pending_reads(); }, "process_pending_reads" );
        else if (_new_read_operation_available_promise)
          _new_read_operation_available_promise->set_value();

        try
        {
          bytes_read = completion_promise->wait();
        }
        catch (...)
        {
          _read_operations_for_next_iteration.remove(&read_operation);
          _read_operations_in_progress.remove(&read_operation);
          throw;
        }
        _unused_read_tokens += read_operation.permitted_length - bytes_read;
      }
      else
        bytes_read = asio::read_some(socket, buffer, length, offset);

      _actual_download_rate.update(bytes_read);

      return bytes_read;
    }

    size_t rate_limiting_group_impl::writesome(boost::asio::ip::tcp::socket& socket, const char* buffer, size_t length)
    {
      return writesome_impl(socket, buffer, length, 0);
    }

    size_t rate_limiting_group_impl::writesome(boost::asio::ip::tcp::socket& socket, const std::shared_ptr<const char>& buffer, size_t length, size_t offset)
    {
      return writesome_impl(socket, buffer, length, offset);
    }

    template <typename BufferType>
    size_t rate_limiting_group_impl::writesome_impl(boost::asio::ip::tcp::socket& socket, const BufferType& buffer, size_t length, size_t offset)
    {
      size_t bytes_written;
      if (_upload_bytes_per_second)
      {
        promise<size_t>::ptr completion_promise(new promise<size_t>("rate_limiting_group_impl::writesome"));
        rate_limited_tcp_write_operation write_operation(socket, buffer, length, offset, completion_promise);
        _write_operations_for_next_iteration.push_back(&write_operation);

        // launch the write processing loop it if isn't running, or signal it to resume if it's paused.
        if (!_process_pending_writes_loop_complete.valid() || _process_pending_writes_loop_complete.ready())
          _process_pending_writes_loop_complete = async([=](){ process_pending_writes(); }, "process_pending_writes");
        else if (_new_write_operation_available_promise)
          _new_write_operation_available_promise->set_value();

        try
        {
          bytes_written = completion_promise->wait();
        }
        catch (...)
        {
          _write_operations_for_next_iteration.remove(&write_operation);
          _write_operations_in_progress.remove(&write_operation);
          throw;
        }
        _unused_write_tokens += write_operation.permitted_length - bytes_written;
      }
      else
        bytes_written = asio::write_some(socket, buffer, length, offset);

      _actual_upload_rate.update(bytes_written);

      return bytes_written;
    }

    void rate_limiting_group_impl::process_pending_reads()
    {
      for (;;)
      {
        process_pending_operations(_last_read_iteration_time, _download_bytes_per_second,
                                   _read_operations_in_progress, _read_operations_for_next_iteration, _read_tokens, _unused_read_tokens);

        _new_read_operation_available_promise = new promise<void>("rate_limiting_group_impl::process_pending_reads");
        try
        {
          if (_read_operations_in_progress.empty())
            _new_read_operation_available_promise->wait();
          else
            _new_read_operation_available_promise->wait(_granularity);
        }
        catch (const timeout_exception&)
        {
        }
        _new_read_operation_available_promise.reset();
      }
    }
    void rate_limiting_group_impl::process_pending_writes()
    {
      for (;;)
      {
        process_pending_operations(_last_write_iteration_time, _upload_bytes_per_second,
                                   _write_operations_in_progress, _write_operations_for_next_iteration, _write_tokens, _unused_write_tokens);

        _new_write_operation_available_promise = new promise<void>("rate_limiting_group_impl::process_pending_writes");
        try
        {
          if (_write_operations_in_progress.empty())
            _new_write_operation_available_promise->wait();
          else
            _new_write_operation_available_promise->wait(_granularity);
        }
        catch (const timeout_exception&)
        {
        }
        _new_write_operation_available_promise.reset();
      }
    }
    void rate_limiting_group_impl::process_pending_operations(time_point& last_iteration_start_time,
                                                              uint32_t& limit_bytes_per_second,
                                                              rate_limited_operation_list& operations_in_progress,
                                                              rate_limited_operation_list& operations_for_next_iteration,
                                                              uint32_t& tokens,
                                                              uint32_t& unused_tokens)
    {
      // lock here for multithreaded
      std::copy(operations_for_next_iteration.begin(),
                operations_for_next_iteration.end(),
                std::back_inserter(operations_in_progress));
      operations_for_next_iteration.clear();

      // find out how much time since our last read/write
      time_point this_iteration_start_time = fc::now();
      if (limit_bytes_per_second) // the we are limiting up/download speed
      {
        microseconds time_since_last_iteration = this_iteration_start_time - last_iteration_start_time;
        if (time_since_last_iteration > seconds(1))
          time_since_last_iteration = seconds(1);
        else if (time_since_last_iteration < microseconds(0))
          time_since_last_iteration = microseconds(0);

        tokens += (uint32_t)((limit_bytes_per_second * time_since_last_iteration.count()) / 1000000);
        tokens += unused_tokens;
        unused_tokens = 0;
        tokens = std::min(tokens, limit_bytes_per_second * _burstiness_in_seconds);

        if (tokens)
        {
          // sort the pending reads/writes in order of the number of bytes they need to write, smallest first
          std::vector<rate_limited_operation*> operations_sorted_by_length;
          operations_sorted_by_length.reserve(operations_in_progress.size());
          for (rate_limited_operation* operation_data : operations_in_progress)
            operations_sorted_by_length.push_back(operation_data);
          std::sort(operations_sorted_by_length.begin(), operations_sorted_by_length.end(), is_operation_shorter());

          // figure out how many bytes each reader/writer is allowed to read/write
          uint32_t bytes_remaining_to_allocate = tokens;
          while (!operations_sorted_by_length.empty())
          {
            uint32_t bytes_permitted_for_this_operation = bytes_remaining_to_allocate / operations_sorted_by_length.size();
            uint32_t bytes_allocated_for_this_operation = std::min<size_t>(operations_sorted_by_length.back()->length, bytes_permitted_for_this_operation);
            operations_sorted_by_length.back()->permitted_length = bytes_allocated_for_this_operation;
            bytes_remaining_to_allocate -= bytes_allocated_for_this_operation;
            operations_sorted_by_length.pop_back();
          }
          tokens = bytes_remaining_to_allocate;

          // kick off the reads/writes in first-come order
          for (auto iter = operations_in_progress.begin(); iter != operations_in_progress.end();)
          {
            if ((*iter)->permitted_length > 0)
            {
              rate_limited_operation* operation_to_perform = *iter;
              iter = operations_in_progress.erase(iter);
              operation_to_perform->perform_operation();
            }
            else
              ++iter;
          }
        }
      }
      else // down/upload speed is unlimited
      {
        // we shouldn't end up here often.  If the rate is unlimited, we should just execute
        // the operation immediately without being queued up.  This should only be hit if
        // we change from a limited rate to unlimited
        for (auto iter = operations_in_progress.begin();
             iter != operations_in_progress.end();)
        {
          rate_limited_operation* operation_to_perform = *iter;
          iter = operations_in_progress.erase(iter);
          operation_to_perform->permitted_length = operation_to_perform->length;
          operation_to_perform->perform_operation();
        }
      }
      last_iteration_start_time = this_iteration_start_time;
    }

  }

  rate_limiting_group::rate_limiting_group(uint32_t upload_bytes_per_second, uint32_t download_bytes_per_second, uint32_t burstiness_in_seconds /* = 1 */) :
    my(new detail::rate_limiting_group_impl(upload_bytes_per_second, download_bytes_per_second, burstiness_in_seconds))
  {
  }

  rate_limiting_group::~rate_limiting_group()
  {
  }

  uint32_t rate_limiting_group::get_actual_upload_rate() const
  {
    return my->_actual_upload_rate.get_average_rate();
  }

  uint32_t rate_limiting_group::get_actual_download_rate() const
  {
    return my->_actual_download_rate.get_average_rate();
  }

  void rate_limiting_group::set_actual_rate_time_constant(microseconds time_constant)
  {
    my->_actual_upload_rate.set_time_constant(time_constant);
    my->_actual_download_rate.set_time_constant(time_constant);
  }

  void rate_limiting_group::set_upload_limit(uint32_t upload_bytes_per_second)
  {
    my->_upload_bytes_per_second = upload_bytes_per_second;
  }

  uint32_t rate_limiting_group::get_upload_limit() const
  {
    return my->_upload_bytes_per_second;
  }

  void rate_limiting_group::set_download_limit(uint32_t download_bytes_per_second)
  {
    my->_download_bytes_per_second = download_bytes_per_second;
  }

  uint32_t rate_limiting_group::get_download_limit() const
  {
    return my->_download_bytes_per_second;
  }

  void rate_limiting_group::add_tcp_socket(tcp_socket* tcp_socket_to_limit)
  {
    tcp_socket_to_limit->set_io_hooks(my.get());
  }

  void rate_limiting_group::remove_tcp_socket(tcp_socket* tcp_socket_to_stop_limiting)
  {
    tcp_socket_to_stop_limiting->set_io_hooks(NULL);
  }


} // namespace fc
