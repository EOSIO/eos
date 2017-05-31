#include <fc/rpc/cli.hpp>
#include <fc/thread/thread.hpp>

#include <iostream>

#ifndef WIN32
#include <unistd.h>
#endif

#ifdef HAVE_READLINE
# include <readline/readline.h>
# include <readline/history.h>
// I don't know exactly what version of readline we need.  I know the 4.2 version that ships on some macs is
// missing some functions we require.  We're developing against 6.3, but probably anything in the 6.x
// series is fine
# if RL_VERSION_MAJOR < 6
#  ifdef _MSC_VER
#   pragma message("You have an old version of readline installed that might not support some of the features we need")
#   pragma message("Readline support will not be compiled in")
#  else
#   warning "You have an old version of readline installed that might not support some of the features we need"
#   warning "Readline support will not be compiled in"
#  endif
#  undef HAVE_READLINE
# endif
# ifdef WIN32
#  include <io.h>
# endif
#endif

namespace fc { namespace rpc {

static std::vector<std::string>& cli_commands()
{
   static std::vector<std::string>* cmds = new std::vector<std::string>();
   return *cmds;
}

cli::~cli()
{
   if( _run_complete.valid() )
   {
      stop();
   }
}

variant cli::send_call( api_id_type api_id, string method_name, variants args /* = variants() */ )
{
   FC_ASSERT(false);
}

variant cli::send_callback( uint64_t callback_id, variants args /* = variants() */ )
{
   FC_ASSERT(false);
}

void cli::send_notice( uint64_t callback_id, variants args /* = variants() */ )
{
   FC_ASSERT(false);
}

void cli::start()
{
   cli_commands() = get_method_names(0);
   _run_complete = fc::async( [&](){ run(); } );
}

void cli::stop()
{
   _run_complete.cancel();
   _run_complete.wait();
}

void cli::wait()
{
   _run_complete.wait();
}

void cli::format_result( const string& method, std::function<string(variant,const variants&)> formatter)
{
   _result_formatters[method] = formatter;
}

void cli::set_prompt( const string& prompt )
{
   _prompt = prompt;
}

void cli::run()
{
   while( !_run_complete.canceled() )
   {
      try
      {
         std::string line;
         try
         {
            getline( _prompt.c_str(), line );
         }
         catch ( const fc::eof_exception& e )
         {
            break;
         }
         std::cout << line << "\n";
         line += char(EOF);
         fc::variants args = fc::json::variants_from_string(line);;
         if( args.size() == 0 )
            continue;

         const string& method = args[0].get_string();

         auto result = receive_call( 0, method, variants( args.begin()+1,args.end() ) );
         auto itr = _result_formatters.find( method );
         if( itr == _result_formatters.end() )
         {
            std::cout << fc::json::to_pretty_string( result ) << "\n";
         }
         else
            std::cout << itr->second( result, args ) << "\n";
      }
      catch ( const fc::exception& e )
      {
         std::cout << e.to_detail_string() << "\n";
      }
   }
}


char * dupstr (const char* s) {
   char *r;

   r = (char*) malloc ((strlen (s) + 1));
   strcpy (r, s);
   return (r);
}

char* my_generator(const char* text, int state)
{
   static int list_index, len;
   const char *name;

   if (!state) {
      list_index = 0;
      len = strlen (text);
   }

   auto& cmd = cli_commands();

   while( list_index < cmd.size() ) 
   {
      name = cmd[list_index].c_str();
      list_index++;

      if (strncmp (name, text, len) == 0)
         return (dupstr(name));
   }

   /* If no names matched, then return NULL. */
   return ((char *)NULL);
}


static char** cli_completion( const char * text , int start, int end)
{
   char **matches;
   matches = (char **)NULL;

#ifdef HAVE_READLINE
   if (start == 0)
      matches = rl_completion_matches ((char*)text, &my_generator);
   else
      rl_bind_key('\t',rl_abort);
#endif

   return (matches);
}


void cli::getline( const fc::string& prompt, fc::string& line)
{
   // getting file descriptor for C++ streams is near impossible
   // so we just assume it's the same as the C stream...
#ifdef HAVE_READLINE
#ifndef WIN32   
   if( isatty( fileno( stdin ) ) )
#else
   // it's implied by
   // https://msdn.microsoft.com/en-us/library/f4s0ddew.aspx
   // that this is the proper way to do this on Windows, but I have
   // no access to a Windows compiler and thus,
   // no idea if this actually works
   if( _isatty( _fileno( stdin ) ) )
#endif
   {
      rl_attempted_completion_function = cli_completion;

      static fc::thread getline_thread("getline");
      getline_thread.async( [&](){
         char* line_read = nullptr;
         std::cout.flush(); //readline doesn't use cin, so we must manually flush _out
         line_read = readline(prompt.c_str());
         if( line_read == nullptr )
            FC_THROW_EXCEPTION( fc::eof_exception, "" );
         rl_bind_key( '\t', rl_complete );
         if( *line_read )
            add_history(line_read);
         line = line_read;
         free(line_read);
      }).wait();
   }
   else
#endif
   {
      std::cout << prompt;
      // sync_call( cin_thread, [&](){ std::getline( *input_stream, line ); }, "getline");
      fc::getline( fc::cin, line );
      return;
   }
}

} } // namespace fc::rpc
