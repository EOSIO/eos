/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#define EOS_ASSERT( expr, exc_type, FORMAT, ... )                \
   FC_MULTILINE_MACRO_BEGIN                                           \
   if( !(expr) )                                                      \
      FC_THROW_EXCEPTION( exc_type, FORMAT, __VA_ARGS__ );            \
   FC_MULTILINE_MACRO_END

#define EOS_THROW( exc_type, FORMAT, ... ) \
    throw exc_type( FC_LOG_MESSAGE( error, FORMAT, __VA_ARGS__ ) );

/**
 * Macro inspired from FC_RETHROW_EXCEPTIONS
 * The main difference here is that if the exception caught isn't of type "eosio::chain::chain_exception"
 * This macro will rethrow the exception as the specified "exception_type"
 */
#define EOS_RETHROW_EXCEPTIONS(exception_type, FORMAT, ... ) \
   catch (eosio::chain::chain_exception& e) { \
      FC_RETHROW_EXCEPTION( e, warn, FORMAT, __VA_ARGS__ ); \
   } catch (fc::exception& e) { \
      exception_type new_exception(FC_LOG_MESSAGE( warn, FORMAT, __VA_ARGS__ )); \
      for (const auto& log: e.get_log()) { \
         new_exception.append_log(log); \
      } \
      throw new_exception; \
   } catch( const std::exception& e ) {  \
      exception_type fce(FC_LOG_MESSAGE( warn, FORMAT" (${what})" ,__VA_ARGS__("what",e.what()))); \
      throw fce;\
   } catch( ... ) {  \
      throw fc::unhandled_exception( \
                FC_LOG_MESSAGE( warn, FORMAT,__VA_ARGS__), \
                std::current_exception() ); \
   }

/**
 * Macro inspired from FC_CAPTURE_AND_RETHROW
 * The main difference here is that if the exception caught isn't of type "eosio::chain::chain_exception"
 * This macro will rethrow the exception as the specified "exception_type"
 */
#define EOS_CAPTURE_AND_RETHROW( exception_type, ... ) \
   catch (eosio::chain::chain_exception& e) { \
      FC_RETHROW_EXCEPTION( e, warn, "", FC_FORMAT_ARG_PARAMS(__VA_ARGS__) ); \
   } catch (fc::exception& e) { \
      exception_type new_exception(e.get_log()); \
      throw new_exception; \
   } catch( const std::exception& e ) {  \
      exception_type fce( \
                FC_LOG_MESSAGE( warn, "${what}: ",FC_FORMAT_ARG_PARAMS(__VA_ARGS__)("what",e.what())), \
                fc::std_exception_code,\
                BOOST_CORE_TYPEID(decltype(e)).name(), \
                e.what() ) ; throw fce;\
   } catch( ... ) {  \
      throw fc::unhandled_exception( \
                FC_LOG_MESSAGE( warn, "",FC_FORMAT_ARG_PARAMS(__VA_ARGS__)), \
                std::current_exception() ); \
   }

#define EOS_DECLARE_OP_BASE_EXCEPTIONS( op_name )                \
   FC_DECLARE_DERIVED_EXCEPTION(                                      \
      op_name ## _validate_exception,                                 \
      eosio::chain::message_validate_exception,                  \
      3040000 + 100 * operation::tag< op_name ## _operation >::value, \
      #op_name "_operation validation exception"                      \
      )                                                               \
   FC_DECLARE_DERIVED_EXCEPTION(                                      \
      op_name ## _evaluate_exception,                                 \
      eosio::chain::message_evaluate_exception,                  \
      3050000 + 100 * operation::tag< op_name ## _operation >::value, \
      #op_name "_operation evaluation exception"                      \
      )

#define EOS_DECLARE_OP_VALIDATE_EXCEPTION( exc_name, op_name, seqnum, msg ) \
   FC_DECLARE_DERIVED_EXCEPTION(                                      \
      op_name ## _ ## exc_name,                                       \
      eosio::chain::op_name ## _validate_exception,                \
      3040000 + 100 * operation::tag< op_name ## _operation >::value  \
         + seqnum,                                                    \
      msg                                                             \
      )

#define EOS_DECLARE_OP_EVALUATE_EXCEPTION( exc_name, op_name, seqnum, msg ) \
   FC_DECLARE_DERIVED_EXCEPTION(                                      \
      op_name ## _ ## exc_name,                                       \
      eosio::chain::op_name ## _evaluate_exception,                \
      3050000 + 100 * operation::tag< op_name ## _operation >::value  \
         + seqnum,                                                    \
      msg                                                             \
      )

#define EOS_RECODE_EXC( cause_type, effect_type ) \
   catch( const cause_type& e ) \
   { throw( effect_type( e.what(), e.get_log() ) ); }


