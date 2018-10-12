#include <eosiolib/eosio.hpp>
#include <eosiolib/action.hpp>
#include <string>
#include <vector>

using eosio::indexed_by;
using eosio::const_mem_fun;
using eosio::contract;
using eosio::multi_index;

using std::string;
using std::vector;

// s is a uint64_t number
uint64_t atoul( const char *s ) {
   uint64_t n = 0;

   while ( isdigit( *s ))
      n = 10 * n + ( *s++ - '0' );

   return n;
}

class P2PInsurance : public contract {
public:
   //构造函数
   using contract::contract;

   //添加联系人
   //@abi action
//   void createinsure( const string contract_id, string Insurance_holder, const string &quantity, const string &Insurance_num, const string &Insurance_company_id, const string &Insurance_company_name, const string &download_url, const string &time, const string &state, const const string &loan_id ) {
//
//      require_auth( eosio::string_to_name( Insurance_company_id.c_str()));
//      require_recipient( eosio::string_to_name( Insurance_holder.c_str()));
//
//   }

   //@abi action
   void repayment( const string &loan_id,
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
                   const string &insurance_num,
                   const string &download_url,
                   const string &insurance_time,
                   const string &state ) {
      require_auth( eosio::string_to_name( insurance_company_id.c_str()));
      require_recipient( eosio::string_to_name( insurance_holder_id.c_str()));
   }


   //@abi action
   void cancelinsure( const string &loan_id, const string &insurance_holder_id, const string &insurance_company_id ) {
      require_auth( eosio::string_to_name( insurance_company_id.c_str()));
      require_recipient( eosio::string_to_name( insurance_holder_id.c_str()));
   }

   //@abi action
   void changestate( const string &loan_id, const string &insurance_holder_id, const string &state, const string &insurance_company_id ) {
      require_auth( eosio::string_to_name( insurance_company_id.c_str()));
      require_recipient( eosio::string_to_name( insurance_holder_id.c_str()));
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
                      const string &beneficiary_order_num,
                      const string &beneficiary_state,
                      const string &insurance_company_id
   ) {
      require_auth( eosio::string_to_name( insurance_company_id.c_str()));
      require_recipient( eosio::string_to_name( beneficiary_id.c_str()));
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
      require_auth( eosio::string_to_name( insurance_company_id.c_str()));
      require_recipient( eosio::string_to_name( beneficiary_id.c_str()));
   }

   //////  add new action

   //@abi action
   void createinsure( const string &loan_id,
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
                      const vector<string> &beneficiary_state ) {
      require_auth( eosio::string_to_name( insurance_company_id.c_str()));
      require_recipient( eosio::string_to_name( insurance_holder_id.c_str()));

      holderTable holder( _self, atoul( loan_id.c_str()));

      holder.emplace( eosio::string_to_name( insurance_company_id.c_str()), [&]( holderinfo &rec ) {
         rec.loan_id = loan_id;
         rec.contract_id = contract_id;
         rec.insurance_holder_id = insurance_holder_id;
         rec.insurance_holder_name = insurance_holder_name;
         rec.insurance_holder_tel = insurance_holder_tel;
         rec.insurance_holder_certificates_type = insurance_holder_certificates_type;
         rec.insurance_holder_certificates_num = insurance_holder_certificates_num;
         rec.insurance_holder_address = insurance_holder_address;
         rec.loan_purposes = loan_purposes;
         rec.loan_principal = loan_principal;
         rec.loan_interest = loan_interest;
         rec.loan_term = loan_term;
         rec.repayment_method = repayment_method;
         rec.premium = premium;
         rec.start_date = start_date;
         rec.end_date = end_date;
         rec.repayment_date = repayment_date;
         rec.insure_quantity = insure_quantity;
         rec.insurance_company_id = insurance_company_id;
         rec.insurance_company_name = insurance_company_name;
         rec.load_confirm_time = load_confirm_time;
         rec.load_confirm_state = load_confirm_state;
      } );

      beneficTable beneficiary( _self, atoul( loan_id.c_str()));

      for ( size_t i = 0; i < beneficiary_id.size(); ++i ) {
         beneficiary.emplace( eosio::string_to_name( insurance_company_id.c_str()), [&]( beneficinfo &rec ) {
            rec.loan_id = loan_id;
            rec.contract_id = contract_id;
            rec.beneficiary_id = beneficiary_id[i];
            rec.beneficiary_name = beneficiary_name[i];
            rec.beneficiary_tel = beneficiary_tel[i];
            rec.beneficiary_certificates_type = beneficiary_certificates_type[i];
            rec.beneficiary_certificates_num = beneficiary_certificates_num[i];
            rec.quantity = quantity[i];
            rec.weitght = weitght[i];
            rec.beneficiary_order_num = beneficiary_order_num[i];
            rec.beneficiary_state = beneficiary_state[i];
            rec.insurance_company_id = insurance_company_id;
         } );
      }
   }

   //@abi action
   void verifypas( const string &loan_id,
                   const string &insurance_company_id,
                   const string &insurance_holder_id,
                   const string &insurance_num,
                   const string &download_url,
                   const string &insurance_time,
                   const string &state ) {
      require_auth( eosio::string_to_name( insurance_company_id.c_str()));
      require_recipient( eosio::string_to_name( insurance_holder_id.c_str()));

      holderTable holder( _self, atoul( loan_id.c_str()));

      for ( auto itr = holder.begin(); itr != holder.end(); ++itr ) {
         SEND_INLINE_ACTION( *this, repayment, { eosio::string_to_name( insurance_company_id.c_str()), N( active ) }, {
            itr->loan_id,
            itr->contract_id,
            itr->insurance_holder_id,
            itr->insurance_holder_name,
            itr->insurance_holder_tel,
            itr->insurance_holder_certificates_type,
            itr->insurance_holder_certificates_num,
            itr->insurance_holder_address,
            itr->loan_purposes,
            itr->loan_principal,
            itr->loan_interest,
            itr->loan_term,
            itr->repayment_method,
            itr->premium,
            itr->start_date,
            itr->end_date,
            itr->repayment_date,
            itr->insure_quantity,
            itr->insurance_company_id,
            itr->insurance_company_name,
            itr->load_confirm_time,
            itr->load_confirm_state,
            insurance_num,
            download_url,
            insurance_time,
            state
         } );
      }

      beneficTable beneficiary( _self, atoul( loan_id.c_str()));

      for ( auto itr = beneficiary.begin(); itr != beneficiary.end(); ++itr ) {
         SEND_INLINE_ACTION( *this, modaddinsure, { eosio::string_to_name( insurance_company_id.c_str()), N( active ) }, {
            itr->loan_id,
            itr->contract_id,
            itr->beneficiary_id,
            itr->beneficiary_name,
            itr->beneficiary_tel,
            itr->beneficiary_certificates_type,
            itr->beneficiary_certificates_num,
            itr->quantity,
            itr->weitght,
            itr->beneficiary_order_num,
            itr->beneficiary_state,
            itr->insurance_company_id
         } );
      }

      clearstate( loan_id );
   }

   //@abi action
   void verifyunpas( const string &loan_id,
                     const string &insurance_company_id,
                     const string &insurance_holder_id,
                     const string &insurance_num,
                     const string &download_url,
                     const string &insurance_time,
                     const string &state ) {
      require_auth( eosio::string_to_name( insurance_company_id.c_str()));
      require_recipient( eosio::string_to_name( insurance_holder_id.c_str()));

      clearstate( loan_id );
   }

   void clearstate( const string &loan_id ) {
      holderTable holder( _self, atoul( loan_id.c_str()));
      for ( auto itr = holder.begin(); itr != holder.end(); ) {
         itr = holder.erase( itr );
      }

      beneficTable beneficiary( _self, atoul( loan_id.c_str()));
      for ( auto itr = beneficiary.begin(); itr != beneficiary.end(); ) {
         itr = beneficiary.erase( itr );
      }
   }

private:
   struct holderinfo {
      string loan_id;
      string contract_id;
      string insurance_holder_id;
      string insurance_holder_name;
      string insurance_holder_tel;
      string insurance_holder_certificates_type;
      string insurance_holder_certificates_num;
      string insurance_holder_address;
      string loan_purposes;
      string loan_principal;
      string loan_interest;
      string loan_term;
      string repayment_method;
      string premium;
      string start_date;
      string end_date;
      string repayment_date;
      string insure_quantity;
      string insurance_company_id;
      string insurance_company_name;
      string load_confirm_time;
      string load_confirm_state;

      uint64_t primary_key() const { return atoul( loan_id.c_str()); }
   };

   struct beneficinfo {
      string loan_id;
      string contract_id;
      string beneficiary_id;
      string beneficiary_name;
      string beneficiary_tel;
      string beneficiary_certificates_type;
      string beneficiary_certificates_num;
      string quantity;
      string weitght;
      string beneficiary_order_num;
      string beneficiary_state;
      string insurance_company_id;

      uint64_t primary_key() const { return eosio::string_to_name( beneficiary_id.c_str()); }
   };

   typedef multi_index<N( holderinfo ), holderinfo> holderTable;
   typedef multi_index<N( beneficinfo ), beneficinfo> beneficTable;
};

EOSIO_ABI( P2PInsurance, ( cancelinsure )( modaddinsure )( moddelinsure )( changestate )
      ( createinsure )( verifypas )( verifyunpas )( repayment ))

