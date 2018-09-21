//
// Created by shengminghui on 04/09/18.
//

#include "insurancePolicy.h"

template<size_t BUFLEN = 1024>
static std::string ErrMsg(const char *format, ...) {
   char buffer[BUFLEN];
   va_list args;
   va_start (args, format);
   vsnprintf(buffer, BUFLEN, format, args);
   va_end (args);
   
   return buffer;
}

// @abi action
void P2PInsurance::createinsure(uint64_t contract_id, const std::string &insurance_holder,
                                const std::string &quantity, const std::string &insurance_num,
                                const std::string &insurance_company_id,
                                const std::string &insurance_company_name,
                                const std::string &download_url, const std::string &time,
                                const std::string &state,
                                uint64_t loan_id) {
   // 标识: 是否需要认证
   
   holderTable table(_self, _self);
   
   auto itr = table.find(contract_id);
   
   // create a insurance
   table.emplace(_self, [&](auto &record) {
      record.contract_id = contract_id;
      record.insurance_holder = insurance_holder;
      record.quantity = quantity;
      record.insurance_num = insurance_num;
      record.insurance_company_id = insurance_company_id;
      record.insurance_company_name = insurance_company_name;
      record.download_url = download_url;
      record.time = time;
      record.state = state;
      record.loan_id = loan_id;
   });
   
}

// @abi action
void P2PInsurance::cancelinsure(uint64_t contract_id,
                                const std::string &insurance_holder,
                                uint64_t loan_id) {
   // 标识: 是否需要认证
   // 标识: 不知道account_name的作用
   holderTable table(_self, _self);
   auto itr = table.find(contract_id);
   
   const char *cancelInsureErrFmt = "Address for contract %ld not found";
   eosio_assert(itr != table.end(), ErrMsg(cancelInsureErrFmt, contract_id).c_str());
   
   const char *matchError = "Contract id and insurance holder do not match";
   eosio_assert(itr->insurance_holder == insurance_holder, ErrMsg(matchError).c_str());
   
   const char *loanMatchError = "Contract id and loan id do not match";
   eosio_assert(itr->loan_id == loan_id, ErrMsg(loanMatchError).c_str());
   
   // delete a insurance by contract id
   table.erase(itr);
}

//@abi action
void P2PInsurance::changestate(uint64_t contract_id,
                               const std::string &insurance_holder,
                               const std::string &state,
                               uint64_t loan_id) {
   // 标识: 是否需要认证
   // 标识: insurance_holder 有什么作用
   
   holderTable table(_self, _self);
   
   auto itr = table.find(contract_id);
   
   const char *changeStateErrFmt = "Address for contract %ld not found";
   eosio_assert(itr != table.end(), ErrMsg(changeStateErrFmt, contract_id).c_str());
   
   const char *matchError = "Contract id and insurance holder do not match";
   eosio_assert(itr->insurance_holder == insurance_holder, ErrMsg(matchError).c_str());
   
   const char *loanMatchError = "Contract id and loan id do not match";
   eosio_assert(itr->loan_id == loan_id, ErrMsg(loanMatchError).c_str());
   
   table.modify(itr, _self, [&](auto &record) {
      record.state = state;
   });
}

// @abi action
void P2PInsurance::modaddinsure(uint64_t contract_id, const std::string &beneficiary,
                                const std::string &quantity, const std::string &weight,
                                const std::string &insurance_num, uint64_t beneficiary_order_num,
                                const std::string &download_url, const std::string &time,
                                const std::string &insurance_holder_id,
                                const std::string &insurance_company_id,
                                const std::string &insurance_company_name,
                                const std::string &beneficiary_state,
                                uint64_t loan_id) {
   // 标识: 是否需要认证
   
   beneficTable table(_self, _self);
   
   auto beneficiary_id = table.available_primary_key();
   
   table.emplace(_self, [&](auto &record) {
      record.beneficiary_id = beneficiary_id;
      record.contract_id = contract_id;
      record.beneficiary = beneficiary;
      record.quantity = quantity;
      record.weight = weight;
      record.insurance_num = insurance_num;
      record.beneficiary_order_num = beneficiary_order_num;
      record.download_url = download_url;
      record.time = time;
      record.insurance_holder_id = insurance_holder_id;
      record.insurance_company_id = insurance_company_id;
      record.insurance_company_name = insurance_company_name;
      record.beneficiary_state = beneficiary_state;
      record.loan_id = loan_id;
      
   });
}

// @abi action
void P2PInsurance::moddelinsure(uint64_t contract_id,
                                const std::string &beneficiary,
                                uint64_t loan_id) {
   // 标识: 是否需要认证
   
   beneficTable table(_self, _self);
   
   auto contractIds = table.get_index<N(contractid)>();
   auto itr = contractIds.lower_bound(contract_id);
   while (itr != contractIds.end() && itr->contract_id == contract_id) {
      if (itr->beneficiary == beneficiary && itr->loan_id == loan_id) {
         itr = contractIds.erase(itr);
      } else {
         ++itr;
      }
   }
}


EOSIO_ABI(P2PInsurance, (createinsure)
      (cancelinsure)
      (modaddinsure)
      (moddelinsure)
      (changestate))
