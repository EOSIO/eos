#include <eosiolib/eosio.hpp>
#include <eosiolib/dispatcher.hpp>
#include <string>

using eosio::contract;
using eosio::indexed_by;
using eosio::const_mem_fun;
using std::string;

class P2PInsurance : public contract {
public:
   //构造函数
   using contract::contract;

   //添加联系人
   //@abi action
   void createinsure( const string &contract_id,
                      const string &Insurance_holder,
                      const string &quantity,
                      const string &Insurance_num,
                      const string &Insurance_company_id,
                      const string &Insurance_company_name,
                      const string &download_url,
                      const string &time,
                      const string &state,
                      uint64_t loan_id ) {
      require_recipient( eosio::string_to_name( Insurance_holder.c_str()));
   }

   //@abi action
   void verifyok() {

   }

   //@abi action
   void verifyfail() {

   }

   //@abi action
   void contract() {

   }

   //@abi action
   void cancelinsure( uint64_t loan_id,
                      const string &Insurance_holder,
                      const string &Insurance_company_name ) {
      require_recipient( eosio::string_to_name( Insurance_holder.c_str()));
   }

   //@abi action
   void modaddinsure( const string &contract_id,
                      const string &Beneficiary,
                      const string &quantity,
                      const string &weitght,
                      const string &Insurance_num,
                      const string &Beneficiary_order_num,
                      const string &download_url,
                      const string &Time,
                      const string &Insurance_holder_id,
                      const string &Insurance_company_id,
                      const string &Insurance_company_name,
                      const string &Beneficiary_state,
                      uint64_t loan_id ) {
      require_recipient( eosio::string_to_name( Beneficiary.c_str()));
   }

   //@abi action
   void moddelinsure( uint64_t loan_id,
                      const string &Beneficiary,
                      const string &Insurance_company_name ) {
      require_recipient( eosio::string_to_name( Beneficiary.c_str()));
   }

   //@abi action
   void changestate( uint64_t loan_id,
                     const string &Insurance_holder,
                     const string &State,
                     const string &Insurance_company_name ) {
      require_auth( eosio::string_to_name( Insurance_company_name.c_str()));
      require_recipient( eosio::string_to_name( Insurance_holder.c_str()));
   }

private:
   struct holderinfo {

   };
};

EOSIO_ABI( P2PInsurance, ( createinsure )( cancelinsure )( modaddinsure )( moddelinsure )( changestate ))

