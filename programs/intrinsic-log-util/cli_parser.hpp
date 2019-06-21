#pragma once

#include <fc/exception/exception.hpp>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include <variant>
#include <memory>
#include <type_traits>

//#include <iosfwd>
#include <iostream>

namespace eosio::cli_parser {

   namespace bfs = boost::filesystem;
   namespace bpo = boost::program_options;

   enum class subcommand_style {
      only_contains_subcommands,
      invokable_and_contains_subcommands,
      terminal_subcommand_with_positional_arguments,
      terminal_subcommand_without_positional_arguments
   };

   struct subcommand_description {
      subcommand_description( const std::string& subcommand_name, const std::string& description )
      :subcommand_name( subcommand_name )
      ,description( description )
      {}

      std::string subcommand_name;
      std::string description;
   };

   struct subcommand_descriptions {
      std::vector<subcommand_description> descriptions;
      bool                                requires_subcommand = false;
   };

   struct positional_description {
      enum argument_restriction {
         required,
         optional,
         repeating
      };

      positional_description( const std::string& positional_name,
                              const std::string& description,
                              argument_restriction restriction )
      :positional_name( positional_name )
      ,description( description )
      ,restriction( restriction )
      {}

      std::string          positional_name;
      std::string          description;
      argument_restriction restriction;
   };

   using positional_descriptions = std::vector<positional_description>;

   struct autocomplete_failure {};
   struct autocomplete_success {};
   struct parse_failure {};

   struct parse_metadata {
      parse_metadata( const std::vector<std::string>& context )
      :subcommand_context( context )
      {}

      /** @pre subcommand_descriptions alternative of other_description must be present */
      void add_subcommand_description( const std::string& subcommand, const std::string& description )
      {
         std::get<subcommand_descriptions>( other_description ).descriptions.emplace_back( subcommand, description );
      }

      void set_positional_descriptions( const positional_descriptions& pos_desc ) {
         other_description.emplace<positional_descriptions>( pos_desc );
      }

      void set_positional_descriptions( positional_descriptions&& pos_desc ) {
         other_description.emplace<positional_descriptions>( std::move(pos_desc) );
      }

      std::vector<std::string>                                       subcommand_context;
      std::string                                                    subcommand_desc;
      std::shared_ptr<bpo::options_description>                      opts_description;
      std::variant<subcommand_descriptions, positional_descriptions> other_description;
      bool                                                           initialization_skipped = false;
   };

   namespace detail {
      template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
      template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

      template<typename T, typename = int>
      struct has_name : std::false_type {};

      template<typename T>
      struct has_name<T, decltype((void) T::name, 0)> : std::true_type {};

      template<typename T, typename = int>
      struct has_description : std::false_type {};

      template<typename T>
      struct has_description<T, decltype((void) T::get_description(), 0)> : std::true_type {};

      template<typename T, typename = int>
      struct has_initialize : std::false_type {};

      template<typename T>
      struct has_initialize<T, decltype((void) std::declval<T>().initialize( std::move( std::declval<bpo::variables_map>() ) ), 0)>
      : std::true_type
      {};

      template<typename Visitor, typename... Args>
      bool maybe_call_visitor( Visitor&& visitor, Args&&... args ) {
         if constexpr( std::is_invocable_v<Visitor, Args...> ) {
            std::forward<Visitor>(visitor)( std::forward<Args>(args)... );
            return true;
         } else {
            return false;
         }
      }

      template<std::size_t I, typename Ret, typename Tuple, typename Extractor, typename Reducer, typename... Args>
      bool reduce_tuple_helper( Ret& accumulator, Tuple&& t,
                                Extractor&& extract, Reducer&& reduce )
      {
         if constexpr( I < std::tuple_size_v<std::decay_t<Tuple>> ) {
            if( !reduce( accumulator, I, extract( std::get<I>( t ) ) ) )
               return false;

            return reduce_tuple_helper<I+1>( accumulator, std::forward<Tuple>(t),
                                             std::forward<Extractor>(extract),
                                             std::forward<Reducer>(reduce) );
         }

         return true;
      }

      template<typename Ret, typename Tuple, typename Extractor, typename Reducer, typename... Args>
      bool reduce_tuple( Ret& accumulator, Tuple&& t,
                         Extractor&& extract, Reducer&& reduce )
      {
         return reduce_tuple_helper<0>( accumulator, std::forward<Tuple>(t),
                                        std::forward<Extractor>(extract),
                                        std::forward<Reducer>(reduce) );
      }

      template<int I, typename Visitor, typename... Ts>
      bool visit_by_name_helper( const std::variant<Ts...>& var, const std::string& subcommand_name, Visitor&& visitor ) {
         using variant_type = std::variant<Ts...>;

         if constexpr( I < std::variant_size_v<variant_type> )
         {
            if constexpr( has_name< std::variant_alternative_t<I, variant_type> >::value ) {
               if( subcommand_name.compare( std::variant_alternative_t<I, variant_type>::name ) == 0 ) {
                  std::forward<Visitor>(visitor)( std::get<I>( var ) );
                  return true;
               }
            }
            return visit_by_name_helper<I+1>( var,
                                              subcommand_name,
                                              std::forward<Visitor>(visitor) );
         } else {
            return false;
         }
      }

      template<typename Visitor, typename... Ts>
      bool visit_by_name( const std::variant<Ts...>& var, const std::string& subcommand_name, Visitor&& visitor ) {
         return visit_by_name_helper<0>( var, subcommand_name, std::forward<Visitor>( visitor ) );
      }

      template<int I, typename Variant, typename Visitor, typename... Args>
      bool construct_variant_by_name_helper( Variant& var,
                                             const std::string& subcommand_name,
                                             Visitor&& visitor,
                                             Args&&... args )
      {
         if constexpr( I < std::variant_size_v<Variant> ) {
            if constexpr( has_name< std::variant_alternative_t<I, Variant> >::value ) {
               if( subcommand_name.compare( std::variant_alternative_t<I, Variant>::name ) == 0 ) {
                  std::forward<Visitor>(visitor)( var.template emplace<I>( std::forward<Args>(args)... ) );
                  return true;
               }
            }
            return construct_variant_by_name_helper<I+1>( var,
                                                          subcommand_name,
                                                          std::forward<Visitor>(visitor),
                                                          std::forward<Args>(args)... );
         } else {
            return false;
         }
      }

      template<typename Variant, typename... Args>
      bool construct_variant_by_name( Variant& var,
                                      const std::string& subcommand_name,
                                      Args&&... args )
      {
         return construct_variant_by_name_helper<0>( var,
                                                     subcommand_name,
                                                     []( auto&& ) {}, // do nothing in visitor
                                                     std::forward<Args>(args)... );
      }

      template<typename Variant, typename Visitor, typename... Args>
      bool construct_and_visit_variant_by_name( Variant& var,
                                                const std::string& subcommand_name,
                                                Visitor&& visitor,
                                                Args&&... args )
      {
         return construct_variant_by_name_helper<0>( var,
                                                     subcommand_name,
                                                     std::forward<Visitor>(visitor),
                                                     std::forward<Args>(args)... );
      }

      template<int I, typename Variant, typename Visitor>
      void static_visit_all_helper( Visitor&& visitor )
      {
         if constexpr( I < std::variant_size_v<Variant> ) {
            if constexpr( has_name< std::variant_alternative_t<I, Variant> >::value ) {
               std::string desc;
               if constexpr( has_description< std::variant_alternative_t<I, Variant> >::value ) {
                  desc = std::variant_alternative_t<I, Variant>::get_description();
               }
               visitor( std::variant_alternative_t<I, Variant>::name, desc );
            }
            return static_visit_all_helper<I+1, Variant>( std::forward<Visitor>(visitor) );
         }
      }

      template<typename Variant, typename Visitor>
      void static_visit_all( Visitor&& visitor ) {
         static_visit_all_helper<0, Variant>( std::forward<Visitor>(visitor) );
      }

      struct no_subcommand {};

      class subcommand_untyped_base {};

      template<typename T, typename... Us>
      class subcommand_base : public subcommand_untyped_base {
         static_assert( (std::is_default_constructible_v<Us> && ...) );
         static_assert( (std::is_base_of_v<subcommand_untyped_base, Us> && ...), "subcommands must derive from subcommand type" );

      protected:
         subcommand_base() = default;
         friend T;

         bpo::options_description _get_options( const std::vector<std::string>& subcommand_context )
         {
            std::string options_name;
            if( subcommand_context.size() > 0 ) {
               for( const auto& s : subcommand_context ) {
                  options_name += s;
                  options_name += " ";
               }
            } else {
               options_name += T::name;
               options_name += " ";
            }
            options_name += "options";

            bpo::options_description options( options_name );
            T& derived = static_cast<T&>( *this );
            derived.set_options( options );
            return options;
         }

         template<typename U=T>
         static std::string _get_description() {
            if constexpr( has_description<U>::value ) {
               return U::get_description();
            } else {
               return {};
            }
         }

         template<typename U=T>
         void _initialize( bpo::variables_map&& vm ) {
            if constexpr( has_initialize<U>::value ) {
               T& derived = static_cast<T&>( *this );
               derived.initialize( std::move(vm) );
            }
         }
      };

      template<typename T, subcommand_style SubcommandStyle, typename... Us>
      class subcommand;

      template<typename T, subcommand_style SubcommandStyle, typename... Us>
      class subcommand
         : public subcommand_base<T,  Us...>
      {
      protected:
         using base_type = subcommand_base<T,  Us...>;
         using subcommands_type = std::variant<no_subcommand, Us...>;

         using _enable = std::enable_if_t<
            SubcommandStyle == subcommand_style::only_contains_subcommands
            || SubcommandStyle == subcommand_style::invokable_and_contains_subcommands
         >;

         static_assert( sizeof...(Us) > 0 );

         subcommands_type _subcommand;

      public:
         static constexpr bool has_subcommands      = true;
         static constexpr bool invokable_subcommand = (SubcommandStyle == subcommand_style::invokable_and_contains_subcommands);

         parse_metadata parse( const std::vector<std::string>& opts, bool skip_initialize,
                               const std::vector<std::string>& subcommand_context = std::vector<std::string>() )
         {
            parse_metadata pm( subcommand_context );
            bpo::options_description subcommand_options{base_type::_get_options(subcommand_context)};

            auto subcommand_options_ptr = std::make_shared<bpo::options_description>( subcommand_options );

            subcommand_options.add_options()
               ("__command", bpo::value<std::string>(), "command to execute" )
               ("__subargs", bpo::value<std::vector<std::string>>(), "arguments for command" )
            ;

            bpo::positional_options_description pos;
            pos.add( "__command", 1 ).add( "__subargs", -1 );

            bpo::parsed_options parsed = bpo::command_line_parser( opts )
                                          .options( subcommand_options )
                                          .positional( pos )
                                          .allow_unregistered()
                                          .run();

            bpo::variables_map vm;

            bpo::variables_map vm_temp;
            bpo::store( parsed, vm_temp );

            if( vm_temp.count( "__command" ) > 0 ) {
               std::string cmd_name = vm_temp["__command"].as<std::string>();

               std::vector<std::string> inner_opts = bpo::collect_unrecognized( parsed.options, bpo::include_positional );
               FC_ASSERT( inner_opts.size() > 0, "unexpected error" );
               FC_ASSERT( *inner_opts.begin() == cmd_name,
                          "unrecognized option '${option}'",
                          ("option", *inner_opts.begin())
               );
               inner_opts.erase( inner_opts.begin() );

               bool success = construct_and_visit_variant_by_name( _subcommand, cmd_name,
                  [&inner_opts, &pm, &cmd_name, &subcommand_options_ptr, &skip_initialize, context = subcommand_context]
                  ( auto&& s ) mutable {
                     context.emplace_back( cmd_name );
                     pm = s.parse( inner_opts, skip_initialize, context );
                     FC_ASSERT( pm.opts_description, "unexpected error" );
                     subcommand_options_ptr->add( *pm.opts_description );
                     pm.opts_description = subcommand_options_ptr;
                  }
               );
               FC_ASSERT(  success, "unrecognized sub-command '${name}' within context: ${context}",
                           ("name", cmd_name)("context", subcommand_context)
               );
               vm = std::move(vm_temp);
            } else {
               bpo::parsed_options parsed2 = bpo::command_line_parser( opts )
                                                .options( *subcommand_options_ptr )
                                                .run();
               bpo::store( parsed2, vm );

               std::get<subcommand_descriptions>( pm.other_description ).requires_subcommand = !invokable_subcommand;

               static_visit_all<std::decay_t<decltype(_subcommand)>>(
                  [&pm]( const char* name, const std::string& desc ) {
                     pm.add_subcommand_description( name, desc );
                  }
               );

               pm.opts_description = std::move(subcommand_options_ptr);
               pm.subcommand_desc  = base_type::_get_description();
               pm.initialization_skipped = skip_initialize;
            }

            if( !skip_initialize ) {
               bpo::notify( vm );
               base_type::_initialize( std::move(vm) );
            }

            return pm;
         }

         const subcommands_type& get_subcommand()const { return _subcommand; }
      };

      template<typename T>
      class subcommand<T, subcommand_style::terminal_subcommand_with_positional_arguments>
         : public subcommand_base<T>
      {
      protected:
         using base_type = subcommand_base<T>;

      public:
         static constexpr bool has_subcommands = false;
         static constexpr bool invokable_subcommand = true;

         parse_metadata parse( const std::vector<std::string>& opts, bool skip_initialize,
                               const std::vector<std::string>& subcommand_context = std::vector<std::string>() )
         {
            parse_metadata pm( subcommand_context );
            pm.subcommand_desc  = base_type::_get_description();
            pm.opts_description = std::make_shared<bpo::options_description>(
                                          base_type::_get_options(subcommand_context) );

            bpo::options_description subcommand_options{ *pm.opts_description };
            positional_descriptions pos_desc;
            const auto& pos = _get_positional_options( subcommand_options, pos_desc );
            pm.set_positional_descriptions( pos_desc );

            bpo::variables_map vm;

            bpo::store( bpo::command_line_parser( opts )
                           .options( subcommand_options )
                           .positional( pos )
                           .run(),
                        vm );

            if( !skip_initialize ) {
               bpo::notify( vm );
               base_type::_initialize( std::move(vm) );
            }
            pm.initialization_skipped = skip_initialize;

            return pm;
         }

      protected:
         bpo::positional_options_description _get_positional_options( bpo::options_description& options,
                                                                      positional_descriptions& pos_desc )
         {
            T& derived = static_cast<T&>( *this );
            return derived.get_positional_options( options, pos_desc );
         }
      };

      template<typename T>
      class subcommand<T, subcommand_style::terminal_subcommand_without_positional_arguments>
         : public subcommand_base<T>
      {
      protected:
         using base_type = subcommand_base<T>;

      public:
         static constexpr bool has_subcommands = false;
         static constexpr bool invokable_subcommand = true;


         parse_metadata parse( const std::vector<std::string>& opts, bool skip_initialize,
                               const std::vector<std::string>& subcommand_context = std::vector<std::string>() )
         {
            parse_metadata pm( subcommand_context );
            pm.subcommand_desc  = base_type::_get_description();
            pm.opts_description = std::make_shared<bpo::options_description>(
                                          base_type::_get_options(subcommand_context) );

            bpo::variables_map vm;

            bpo::store( bpo::command_line_parser( opts )
                           .options( *pm.opts_description )
                           .run(),
                        vm );

            if( !skip_initialize ) {
               bpo::notify( vm );
               base_type::_initialize( std::move(vm) );
            }
            pm.initialization_skipped = skip_initialize;

            return pm;
         }
      };

   } /// namespace detail

   template<typename T, subcommand_style SubcommandStyle, typename... Us>
   using subcommand = eosio::cli_parser::detail::subcommand<T, SubcommandStyle, Us...>;

   bpo::positional_options_description
   build_positional_descriptions( const bpo::options_description& options,
                                  positional_descriptions& pos_desc,
                                  const std::vector<std::string>& required_pos_names,
                                  const std::vector<std::string>& optional_pos_names = std::vector<std::string>(),
                                  const std::string& repeat_name = std::string{}
                                )
   {
      bpo::positional_options_description pos;

      auto add_positional = [&pos, &pos_desc, &options]( const std::string& pos_name,
                                                         positional_description::argument_restriction restriction )
      {
         pos.add( pos_name.c_str(), (restriction == positional_description::repeating ? -1 : 1) );
         const bpo::option_description* opt = options.find_nothrow( pos_name, false );
         if( opt ) {
            pos_desc.emplace_back( pos_name, opt->description(), restriction );
         } else {
            pos_desc.emplace_back( pos_name, "", restriction );
         }
      };

      for( const auto& n : required_pos_names ) {
         add_positional( n, positional_description::required );
      }

      for( const auto& n : optional_pos_names ) {
         add_positional( n, positional_description::optional );
      }

      if( repeat_name.size() > 0 ) {
         add_positional( repeat_name, positional_description::repeating );
      }

      return pos;
   }

   void print_subcommand_help( std::ostream& stream, const std::string& program_name, const parse_metadata& pm ) {
      const subcommand_descriptions* subcommands = std::get_if<subcommand_descriptions>( &pm.other_description );
      bool subcommands_present = (subcommands && subcommands->descriptions.size() > 0);

      const positional_descriptions* positionals = std::get_if<positional_descriptions>( &pm.other_description );
      bool positionals_present = (positionals && positionals->size() > 0);

      auto formatting_width = pm.opts_description->get_option_column_width() - 2;

      std::string subcommand_usage;
      for( const auto& s : pm.subcommand_context ) {
         subcommand_usage += " ";
         subcommand_usage += s;
      }
      if( subcommands_present ) {
         if( subcommands->requires_subcommand )
            subcommand_usage += " SUBCOMMAND";
         else
            subcommand_usage += " [SUBCOMMAND]";
      }

      if( pm.subcommand_desc.size() > 0 ) {
         stream << pm.subcommand_desc << std::endl;
      }

      stream   << "Usage: " << program_name
               << subcommand_usage
               << " [OPTIONS]";

      if( positionals_present ) {
         for( const auto& p : *positionals ) {
            bool optional_arg = (p.restriction == positional_description::optional);
            stream << " ";
            if( optional_arg ) stream << "[";
            stream << p.positional_name;
            if( optional_arg ) stream << "]";
            if( p.restriction == positional_description::repeating ) stream << "...";
         }
      }

      stream << std::endl << std::endl;

      if( positionals_present ) {
         stream << "Positionals:" << std::endl;

         for( const auto& p : *positionals ) {
            stream << "  ";
            auto orig_width = stream.width( formatting_width );
            stream << std::left << p.positional_name;
            stream.width( orig_width );
            stream << p.description << std::endl;
         }

         stream << std::endl;
      }

      pm.opts_description->print( stream );

      if( subcommands_present ) {
         stream << std::endl;

         std::string subcommands_title;
         if( pm.subcommand_context.size() > 0 ) {
            for( const auto& s : pm.subcommand_context ) {
               subcommands_title += s;
               subcommands_title += " ";
            }
            subcommands_title += "subcommands:";
         } else {
            subcommands_title = "Subcommands:";
         }

         stream << subcommands_title << std::endl;
         for( const auto& s : subcommands->descriptions ) {
            stream << "  ";
            auto orig_width = stream.width( formatting_width );
            stream << std::left << s.subcommand_name;
            stream.width( orig_width );
            stream << s.description << std::endl;
         }
      }
   }

   bool print_autocomplete_suggestions( std::ostream& stream, const parse_metadata& pm, const std::string& last_token ) {
      bool printed_something = false;

      if( std::holds_alternative<subcommand_descriptions>( pm.other_description ) ) {
         for( const auto& s : std::get<subcommand_descriptions>( pm.other_description ).descriptions ) {
            if( s.subcommand_name.find( last_token ) != 0 ) continue;
            if( printed_something )
               stream << " ";
            stream << s.subcommand_name;
            printed_something = true;
         }
      }

      if( last_token.size() > 0 ) {
         for( const auto& opt : pm.opts_description->options() ) {
            //if( opt->match( last_token, true, false, false ) == bpo::option_description::no_match ) continue;
            // The option_description::match function doesn't seem to work.

            const auto& short_name = opt->canonical_display_name( bpo::command_line_style::allow_dash_for_short );
            if( short_name.size() > 0 && short_name[0] == '-' && short_name.find( last_token ) == 0 ) {
               if( printed_something )
                  stream << " ";
               stream << short_name;
               printed_something = true;
            }

            const auto& long_name = opt->canonical_display_name( bpo::command_line_style::allow_long );
            if( long_name.size() > 0 && long_name[0] == '-' && long_name.find( last_token ) == 0 ) {
               if( printed_something )
                  stream << " ";
               stream << long_name;
               printed_something = true;
            }
         }
      }

      return printed_something;
   }

   template<typename T>
   std::optional<parse_metadata>
   parse_program_options( T& root, const std::vector<std::string>& opts,
                          bool skip_initialize = false, bool avoid_throwing = false )
   {
      try {
         return root.parse( opts, skip_initialize );
      } catch( ... ) {
         if( !avoid_throwing ) throw;
      }

      return {};
   }

   template<typename T>
   std::variant<autocomplete_failure, autocomplete_success, parse_failure, parse_metadata>
   parse_program_options_extended( T& root, int argc, const char** argv,
                                   std::ostream& out_stream, std::ostream* err_stream = nullptr,
                                   bool avoid_throwing = false )
   {
      if( argc <= 0 ) {
         if( avoid_throwing ) return parse_failure{};
         FC_ASSERT( false, "command line arguments should at least include program name" );
      }

      std::string program_name{ bfs::path( argv[0] ).filename().generic_string() };

      bool autocomplete_mode = false;

      std::vector<std::string> opts;
      opts.reserve( argc - 1 );
      for( int i = 1; i < argc; ++i ) {
         opts.emplace_back( std::string( *(argv + i) ) );
         if( opts.back().compare( "-" ) == 0 ) {
            if( autocomplete_mode ) return autocomplete_failure{};
            if( avoid_throwing ) return parse_failure{};
            FC_ASSERT( false, "- is not allowed by itself in the command line arguments" );
         }
         if( opts.back().compare( "--" ) == 0 ) {
            if( i != 1 ) {
               if( autocomplete_mode ) return autocomplete_failure{};
               if( avoid_throwing ) return parse_failure{};
               FC_ASSERT( false, "-- is not allowed by itself in the command line arguments" );
            }
            opts.pop_back();
            autocomplete_mode = true;
         }
      }

      if( !autocomplete_mode ) {
         const auto pm = parse_program_options( root, opts, false, avoid_throwing );
         if( !pm ) return parse_failure{};
         return *pm;
      }

      // Autocomplete helper logic:

      std::string last_token;
      if( opts.size() > 0 ) {
         if( opts.back().size() == 0 ) {
            opts.pop_back();
         } else {
            last_token = opts.back();
         }
      }

      if( last_token.size() > 0 ) {
         if( opts.size() == 0 ) return autocomplete_failure{};
         opts.pop_back();
      }

      auto pm = parse_program_options( root, opts, true, true );
      if( !pm ) return autocomplete_failure{};

      if( last_token.size() > 0 ) {
         if( print_autocomplete_suggestions( out_stream, *pm, last_token ) ) {
            out_stream << std::endl;
         } else {
            return autocomplete_failure{};
         }
      }

      if( err_stream ) {
         *err_stream << std::endl;
         print_subcommand_help( *err_stream, program_name, *pm );
      }

      return autocomplete_success{};
   }

   enum class dispatch_status {
      successful_dispatch,
      no_handler_for_terminal_subcommand,
      no_handler_for_nonterminal_subcommand
   };

   namespace detail {

      template<typename Ret, typename Reducer, typename Visitor, typename LastArg, typename... OtherArgs>
      auto dispatch_helper( Reducer&& reduce, Visitor&& visitor, const LastArg& last_arg, const OtherArgs&... other_args )
      -> std::enable_if_t<
            !std::is_same_v<std::decay_t<LastArg>, no_subcommand>,
            std::pair<dispatch_status, Ret>
         >
      {
         static_assert( std::is_convertible_v<std::invoke_result_t<Reducer, std::tuple<const OtherArgs&..., const LastArg&>>, Ret> );

         dispatch_status status = dispatch_status::no_handler_for_terminal_subcommand;

         if constexpr( LastArg::has_subcommands ) {
            return std::visit([&]( const auto& next_subcommand ) {
               return dispatch_helper<Ret>( std::forward<Reducer>(reduce),
                                            std::forward<Visitor>(visitor),
                                            next_subcommand,
                                            other_args..., last_arg );
            }, last_arg.get_subcommand() );
         } else {
            if( maybe_call_visitor( std::forward<Visitor>(visitor), other_args..., last_arg ) )
               status = dispatch_status::successful_dispatch;
         }

         return {status, reduce( std::make_tuple( std::cref(other_args)..., std::cref(last_arg) ) )};
      }

      template<typename Ret, typename Reducer, typename Visitor, typename LastArg, typename... OtherArgs>
      auto dispatch_helper( Reducer&& reduce, Visitor&& visitor, const LastArg& last_arg, const OtherArgs&... other_args )
      -> std::enable_if_t<
            std::is_same_v<std::decay_t<LastArg>, no_subcommand>,
            std::pair<dispatch_status, Ret>
         >
      {
         static_assert( std::is_convertible_v<std::invoke_result_t<Reducer, std::tuple<const OtherArgs&...>>, Ret> );

         dispatch_status status = dispatch_status::no_handler_for_nonterminal_subcommand;

         if( maybe_call_visitor( std::forward<Visitor>(visitor), other_args... ) )
            status = dispatch_status::successful_dispatch;

         return {status, reduce( std::make_tuple( std::cref(other_args)... )  )};
      }

      template<typename... Args>
      std::string get_subcommand_string( const std::tuple<const Args&...>& t ) {
         static_assert( (has_name<Args>::value && ...) );

         std::string result;

         bool completed = reduce_tuple( result, t,
            []( auto&& v ) -> std::string { return std::decay_t<decltype(v)>::name; },
            []( std::string& accumulator, std::size_t index, const std::string& v ) -> bool {
               if( index != 0 ) {
                  if( index > 1 ) {
                     accumulator += " ";
                  }
                  accumulator += v;
               }
               return true;
            }
         );

         if( !completed ) return {};

         return result;
      }

   } /// namespace detail

   template<typename Dispatcher, typename Root>
   auto dispatch( Dispatcher&& dispatcher, const Root& root ) -> std::pair<dispatch_status, std::string> {
      return detail::dispatch_helper<std::string>(
         []( const auto& t ) { return detail::get_subcommand_string(t); },
         std::forward<Dispatcher>(dispatcher),
         root
      );
   }

} /// namespace eosio::cli_parser

namespace eosio {
   using subcommand_style = cli_parser::subcommand_style;

   template<typename T, subcommand_style SubcommandStyle, typename... Us>
   using subcommand = eosio::cli_parser::subcommand<T, SubcommandStyle, Us...>;
}
