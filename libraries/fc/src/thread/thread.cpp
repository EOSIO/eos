#include <fc/thread/thread.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/mem_fun.hpp>

#if defined(__linux__) && !defined(NDEBUG)
# include <pthread.h>
static void set_thread_name(const char* threadName)
{ pthread_setname_np(pthread_self(), threadName); }
#elif defined(__APPLE__) && !defined(NDEBUG)
# include <pthread.h>
static void set_thread_name(const char* threadName)
{ pthread_setname_np(threadName); }
#else// do nothing in release mode
static void set_thread_name(const char* threadName)
{}
#endif

namespace fc {
   using namespace boost::multi_index;

   struct by_time;
   typedef multi_index_container <
      std::shared_ptr<scheduled_task>,
      indexed_by<
         ordered_non_unique< tag<by_time>, const_mem_fun< scheduled_task, time_point, &scheduled_task::get_scheduled_time > >
      >
   > scheduled_task_index;

   thread*& current_thread() {
      static __thread thread* t = nullptr;
      return t;
   }


   class thread_detail {
      public:
        thread_detail( fc::thread& t ):fc_thread(t) { 
            this_id = std::this_thread::get_id();
            current_thread() = &t;
            task_in_queue = nullptr;
        }
        ~thread_detail(){ }

        fc::thread&                       fc_thread;
        std::thread::id                   this_id;
        std::thread*                      std_thread = nullptr;
        boost::fibers::promise<void>      exit_promise;

        std::string                       name;

        boost::fibers::condition_variable task_ready;
        boost::fibers::mutex              task_ready_mutex;
        std::atomic<detail::task*>        task_in_queue;

        detail::task*                    _queue     = nullptr;
        detail::task*                    _queue_end = nullptr;
        bool                             _running   = false;
        bool                             _done      = false;

        scheduled_task_index             _scheduled;


        void async_task( detail::task* t ) {
           idump((name));
           if( _done ) {
              delete t;
              throw std::runtime_error( "attempt to async task on thread that has quit" );
           }

           auto stale_head = task_in_queue.load(std::memory_order_relaxed);
           do { t->next = stale_head;
           }while( !task_in_queue.compare_exchange_weak( stale_head, t, std::memory_order_release ) );

           if( !stale_head ) 
           {
               dlog( "----grabbing ready mutex" );
               std::unique_lock<boost::fibers::mutex> lock(task_ready_mutex);
               dlog("--- got ready mutex, notify one" );
               task_ready.notify_one();
           }
        }

        bool exec_next_task() {
           auto itr = _scheduled.begin();
           if( _scheduled.end() != itr && (*itr)->get_scheduled_time() < fc::time_point::now() ) {
              idump(((*itr)->get_scheduled_time()));
              auto tsk = *itr;
              _scheduled.erase(itr);
              tsk->exec();
              return true;
           }

           if( !_queue ) 
           {
              move_newly_scheduled_tasks_to_task_queue();
              if( !_queue ) return false;
           }

           auto tmp = _queue;
           _queue = _queue->next;
           if( !_queue ) 
              _queue_end = nullptr;

           tmp->exec();
           delete tmp;

           return true;
        }

        /**
         *  Start a new fiber which will process tasks until it 
         *  blocks on some event, at that time it should resume
         *  the exec() loop which will look for new tasks. If there
         *  are no new tasks then it will block on a wait condition
         *  until a new task comes in.
         *
         */
        void exec() {
           _running = true;
           while( !_done ) {
              move_newly_scheduled_tasks_to_task_queue();
              if( _queue || _scheduled.size() ) {

#if 1
                 boost::fibers::async( boost::fibers::launch::dispatch, [this](){ while( exec_next_task() ){} } );
#else
             //    elog( "creating new fiber... " );
                 static int tmp = 0;
                 ++tmp;
                 /**
                  *  First we execute the task, then delete it, and
                  *  finally look for other tasks to execute, and
                  *  exit when there are no more tasks in the queue
                  */
                 boost::fibers::fiber fib( boost::fibers::launch::dispatch, [this,t=tmp](){
                  //   wlog( "starting new fiber... ${d}", ("d",int64_t(t)) );
                     while( exec_next_task() ){}
                  //   dlog( "exit fiber... ${d}", ("d",int64_t(t)) );
                 });
                 fib.detach();
 #endif

              } else {
                 //ilog( "grabbing task_read_mutex..." );
                 std::unique_lock<boost::fibers::mutex> lock(task_ready_mutex);
                 move_newly_scheduled_tasks_to_task_queue();
                 if( !(_queue || _scheduled.size()) ) {
                    if( !_scheduled.size() ) {
              //         wlog( "waiting until next event" );
                       task_ready.wait( lock );
               //        ilog( "wake up..." );
                    } else {
                //       wlog( "waiting for ${t} or until next event", ("t", (*_scheduled.begin())->get_scheduled_time() - fc::time_point::now() ));
                       task_ready.wait_until( lock, std::chrono::system_clock::time_point(std::chrono::microseconds( (*_scheduled.begin())->get_scheduled_time().time_since_epoch().count())) );
                 //      wlog( "done waiting... " );
                    }
                 }
              }
           }
    //       ilog( "exec done" );
           _running = false;
        }

        void move_newly_scheduled_tasks_to_task_queue()
        {
            // first, if there are any new tasks on 'task_in_queue', which is tasks that 
            // have been just been async or scheduled, but we haven't processed them.
            // move them into the task_sch_queue or task_pqueue, as appropriate
            
            //DLN: changed from memory_order_consume for boost 1.55.
            //This appears to be safest replacement for now, maybe
            //can be changed to relaxed later, but needs analysis.
            auto pending_list = task_in_queue.exchange(0, std::memory_order_seq_cst);
            if( !pending_list ) return;
            
            /** reverse the list */
            detail::task* cur = pending_list;
            detail::task* prev = nullptr;
            detail::task* next = nullptr;
            detail::task* new_end = cur;
            
            while( cur != nullptr ) {
              next = cur->next;
              cur->next = prev;
              prev = cur;
              cur = next;
            }
            
            /** insert the list to the current queue */
            if( !_queue ) {
               _queue = prev;
            } else {
               _queue_end->next = prev;
            }
            _queue_end = new_end;
        }
   };

   namespace detail {
      void thread_detail_cancel( thread& t, scheduled_task* stask ) {
         t.async( [tptr=&t,stask](){
            for( auto itr = tptr->my->_scheduled.begin(); itr != tptr->my->_scheduled.end(); ++itr ) {
              if( itr->get() == stask ) {
                tptr->my->_scheduled.erase(itr);
                return;
              }
            }
         });
      }
   }


   thread::thread( const std::string& name ) {
      boost::fibers::promise<void>  prom;
      auto stdt = new std::thread( [this,name,&prom]() {
         my = new thread_detail( *this );
         my->name = name;
         prom.set_value();
         set_thread_name( name.c_str() );
         this->exec();
         //elog( "exit thread" );
         my->exit_promise.set_value();
      });
      prom.get_future().wait();
      my->std_thread = stdt;
   }
   thread::thread( thread&& mv )
   :my(mv.my){
      mv.my = nullptr;
   }

   thread::thread( thread_detail* d ) {
      my = new thread_detail(*this);
   }

   thread::~thread() {
      delete my->std_thread;
      delete my;
   }

   const string& thread::name()const {
      return my->name;
   }

   thread& thread::current() {
      auto cur = current_thread();
      if( cur ) return *cur;
      return *(new thread( (thread_detail*)nullptr ));
   }

   void thread::quit() {
      if( !my->_done && my->_running )
         async( [&](){  my->_done = true; }, "thread::quit" ).wait();
   }

   bool thread::is_running()const {
      return !my->_done;
   }

   bool thread::is_current()const {
      return this == &current();
   }

   void thread::set_name( const string& n ) {
      //this->async( [=]() {
         my->name = n;
         set_thread_name( my->name.c_str() );
      //}).wait();
   }
   void thread::exec() { 
      if( this != &current() ) elog( "exec called from wrong thread" );
      else my->exec(); 
   }

   void thread::async_task( detail::task* t ) { 
      my->async_task(t); 
      if( !my->_running /*&& this == &current()*/ ) {
         my->_running = true;
         boost::fibers::async( boost::fibers::launch::post, [this](){ my->exec(); } );
         /*
         my->exec();

          boost::fibers::fiber fib( boost::fibers::launch::post, [&](){ 
               elog( "STARTING FIBER to call exec()" );
               exec();
               elog( "EXITING FIBER CALLING EXEC" );
          } );
          fib.detach();
          */
      }
   }
   void thread::schedule( const std::shared_ptr<scheduled_task>& stask ) {
      async( [=]() {
         my->_scheduled.insert( stask );
      });
   }

   void thread::join() {
      quit();
      if( my->std_thread ) {
         my->exit_promise.get_future().wait();
         my->std_thread->join();
      }
   }

} // namespace fc
