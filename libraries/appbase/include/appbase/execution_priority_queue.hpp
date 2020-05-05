#pragma once
#include <boost/asio.hpp>

#include <queue>

namespace appbase {
// adapted from: https://www.boost.org/doc/libs/1_69_0/doc/html/boost_asio/example/cpp11/invocation/prioritised_handlers.cpp

struct priority {
   static constexpr int lowest      = std::numeric_limits<int>::min();
   static constexpr int low         = 10;
   static constexpr int medium_low  = 25;
   static constexpr int medium      = 50;
   static constexpr int medium_high = 75;
   static constexpr int high        = 100;
   static constexpr int highest     = std::numeric_limits<int>::max();
};

class execution_priority_queue : public boost::asio::execution_context
{
public:

   template <typename Function>
   void add(int priority, Function function)
   {
      std::unique_ptr<queued_handler_base> handler(new queued_handler<Function>(priority, --order_, std::move(function)));

      handlers_.push(std::move(handler));
   }

   void execute_all()
   {
      while (!handlers_.empty()) {
         handlers_.top()->execute();
         handlers_.pop();
      }
   }

   bool execute_highest()
   {
      if( !handlers_.empty() ) {
         handlers_.top()->execute();
         handlers_.pop();
      }

      return !handlers_.empty();
   }

   size_t size() { return handlers_.size(); }

   class executor
   {
   public:
      executor(execution_priority_queue& q, int p)
            : context_(q), priority_(p)
      {
      }

      execution_priority_queue& context() const noexcept
      {
         return context_;
      }

      template <typename Function, typename Allocator>
      void dispatch(Function f, const Allocator&) const
      {
         context_.add(priority_, std::move(f));
      }

      template <typename Function, typename Allocator>
      void post(Function f, const Allocator&) const
      {
         context_.add(priority_, std::move(f));
      }

      template <typename Function, typename Allocator>
      void defer(Function f, const Allocator&) const
      {
         context_.add(priority_, std::move(f));
      }

      void on_work_started() const noexcept {}
      void on_work_finished() const noexcept {}

      bool operator==(const executor& other) const noexcept
      {
         return &context_ == &other.context_ && priority_ == other.priority_;
      }

      bool operator!=(const executor& other) const noexcept
      {
         return !operator==(other);
      }

   private:
      execution_priority_queue& context_;
      int priority_;
   };

   template <typename Function>
   boost::asio::executor_binder<Function, executor>
   wrap(int priority, Function&& func)
   {
      return boost::asio::bind_executor( executor(*this, priority), std::forward<Function>(func) );
   }

private:
   class queued_handler_base
   {
   public:
      queued_handler_base( int p, size_t order )
            : priority_( p )
            , order_( order )
      {
      }

      virtual ~queued_handler_base() = default;

      virtual void execute() = 0;

      int priority() const { return priority_; }
      // C++20
      // friend std::weak_ordering operator<=>(const queued_handler_base&,
      //                                       const queued_handler_base&) noexcept = default;
      friend bool operator<(const queued_handler_base& a,
                            const queued_handler_base& b) noexcept
      {
         return std::tie( a.priority_, a.order_ ) < std::tie( b.priority_, b.order_ );
      }

   private:
      int priority_;
      size_t order_;
   };

   template <typename Function>
   class queued_handler : public queued_handler_base
   {
   public:
      queued_handler(int p, size_t order, Function f)
            : queued_handler_base( p, order )
            , function_( std::move(f) )
      {
      }

      void execute() override
      {
         function_();
      }

   private:
      Function function_;
   };

   struct deref_less
   {
      template<typename Pointer>
      bool operator()(const Pointer& a, const Pointer& b) noexcept(noexcept(*a < *b))
      {
         return *a < *b;
      }
   };

   std::priority_queue<std::unique_ptr<queued_handler_base>, std::deque<std::unique_ptr<queued_handler_base>>, deref_less> handlers_;
   std::size_t order_ = std::numeric_limits<size_t>::max(); // to maintain FIFO ordering in queue within priority
};

} // appbase
