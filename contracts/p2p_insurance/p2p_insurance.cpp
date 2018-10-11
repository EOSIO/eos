#include <eosiolib/eosio.hpp>
#include <string>
#include <vector>

using eosio::indexed_by;
using eosio::const_mem_fun;
using eosio::contract;

using std::string;
using std::vector;

class P2PInsurance : public contract {
public:
   //构造函数
   using contract::contract;

   //添加联系人
   //@abi action
   void createinsure( const string contract_id, string Insurance_holder, const string &quantity, const string &Insurance_num, const string &Insurance_company_id, const string &Insurance_company_name, const string &download_url, const string &time, const string &state, const const string &loan_id ) {

      require_auth( eosio::string_to_name( Insurance_company_id.c_str()));
      require_recipient( eosio::string_to_name( Insurance_holder.c_str()));

   }

   //@abi action
   void cancelinsure( const const string &loan_id, string Insurance_holder, string Insurance_company_id ) {
      require_auth( eosio::string_to_name( Insurance_company_id.c_str()));
      require_recipient( eosio::string_to_name( Insurance_holder.c_str()));
   }

   //@abi action
//   void modaddinsure( const string contract_id, string Beneficiary, const string quantity, const string weitght, const string Insurance_num, const string Beneficiary_order_num, const string download_url, const string Time, const string Insurance_holder_id, const string Insurance_company_id, const string Insurance_company_name, const string Beneficiary_state, const const string & loan_id ) {
//      require_auth( eosio::string_to_name( Insurance_company_id.c_str()));
//      require_recipient( eosio::string_to_name( Beneficiary.c_str()));
//   }

   //@abi action
   void modaddinsure( const string &loan_id,
                      const string &contract_id,
                      const string &beneficiary_id,
                      const string &beneficiary_name,
                      const string &beneficiary_tel,
                      const string &beneficiary_certificates_type,
                      const string &beneficiary_certificates_num,
                      const string &quantity,
                      const string &weitght,
                      const string &insurance_num,
                      const string &beneficiary_order_num,
                      const string &insurance_holder,
                      const string &insurance_company_id,
                      const string &insurance_company_name,
                      const string &download_url,
                      const string &time,
                      const string &beneficiary_state ) {

   }

   //@abi action
//   void moddelinsure( const const string & loan_id, string Beneficiary, string Insurance_company_id ) {
//      require_auth( eosio::string_to_name( Insurance_company_id.c_str()));
//      require_recipient( eosio::string_to_name( Beneficiary.c_str()));
//   }

   //@abi action
   void moddelinsure( const string &loan_id,
                      const string &beneficiary_id,
                      const string &time,
                      const string &insurance_company_id ) {

   }

   //@abi action
   void changestate( const const string &loan_id, string Insurance_holder, const string State, string Insurance_company_id ) {
      require_auth( eosio::string_to_name( Insurance_company_id.c_str()));
      require_recipient( eosio::string_to_name( Insurance_holder.c_str()));
   }

   //////  add new action

   //@abi action
   void addinsure( const string &loan_id,
                   const string &contract_id,
                   const string &insurance_holder_id,
                   const string &insurance_holder_name,
                   const string &insurance_holder_tel,
                   const string &insurance_holder_certificates_type,
                   const string &insurance_holder_certificates_num,
                   const string &insurance_holder_address,
                   const string &loan_purposes,
                   const string &loan_principal,
                   const string &loan_interest,
                   const string &loan_term,
                   const string &repayment_method,
                   const string &premium,
                   const string &start_date,
                   const string &end_date,
                   const string &repayment_date,
                   const string &insure_quantity,
                   const string &insurance_company_id,
                   const string &insurance_company_name,
                   const string &load_confirm_time,
                   const string &load_confirm_state,
                   const vector<string> &beneficiary_id,
                   const vector<string> &beneficiary_name,
                   const vector<string> &beneficiary_tel,
                   const vector<string> &beneficiary_certificates_type,
                   const vector<string> &beneficiary_certificates_num,
                   const vector<string> &quantity,
                   const vector<string> &weitght,
                   const vector<string> &beneficiary_order_num,
                   const vector<string> &download_url,
                   const vector<string> &time,
                   const vector<string> &beneficiary_state ) {

   }

   //@abi action
   void verifypas( const string &loan_id,
                   const string &insurance_company_id,
                   const string &insurance_holder_id,
                   const string &insurance_num,
                   const string &download_url,
                   const string &time,
                   const string &state ) {

   }

   //@abi action
   void verifyunpas( const string &loan_id,
                     const string &insurance_company_id,
                     const string &insurance_holder_id,
                     const string &insurance_num,
                     const string &download_url,
                     const string &time,
                     const string &state ) {

   }

   //@abi action
   void repayment() {
      // contract
      // beneficiary
   }

   //abi action
   void contract( const string &loan_id,
                  const string &insurance_num,
                  const string &contract_id,
                  const string &quantity,
                  const string &insurance_company_id,
                  const string &insurance_company_name,
                  const string &,
                  const string &insurance_holder_id,
                  const string &insurance_holder_name,
                  const string &insurance_holder_tel,
                  const string &insurance_holder_certificates_type,
                  const string &insurance_holder_certificates_num,
                  const string &insurance_holder_address,
                  const string &loan_purposes,
                  const string &loan_principal,
                  const string &loan_interest,
                  const string &loan_term,
                  const string &repayment_method,
                  const string &premium,
                  const string &start_date,
                  const string &end_date,
                  const string &repayment_date ) {

   }

   //abi action
   void beneficiary( const string &contract_id,
                     const string &beneficiary,
                     const string &quantity,
                     const string &weitght,
                     const string &beneficiary_state ) {

   }

private:

   struct holderinfo {

   };

};

EOSIO_ABI( P2PInsurance, ( createinsure )( cancelinsure )( modaddinsure )( moddelinsure )
( changestate ))

