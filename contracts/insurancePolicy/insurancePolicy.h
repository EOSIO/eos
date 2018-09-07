//
// Created by shengminghui on 05/09/18.
//
#ifndef EOSIO_INSURANCEPOLICY_H
#define EOSIO_INSURANCEPOLICY_H

#include <eosiolib/eosio.hpp>
#include <string>

using eosio::multi_index;

class P2PInsurance : public eosio::contract {
public:
	// constructor
	explicit P2PInsurance(account_name self) : contract(self) {}

	// @abi action
	void createinsure(uint64_t contract_id,
										const std::string &insurance_holder,
										const std::string &quantity,
										const std::string &insurance_num,
										const std::string &insurance_company_id,
										const std::string &insurance_company_name,
										const std::string &download_url,
										const std::string &time,
										const std::string &state,
										uint64_t loan_id);

	// @abi action
	void cancelinsure(uint64_t contract_id, const std::string &account_name, uint64_t loan_id);

	//@abi action
	void changestate(uint64_t contract_id, const std::string &insurance_holder,
									 const std::string &state, uint64_t loan_id);

	// @abi action
	void modaddinsure(uint64_t contract_id,
										const std::string &beneficiary,
										const std::string &quantity,
										const std::string &weight,
										const std::string &insurance_num,
										uint64_t beneficiary_order_num,
										const std::string &download_url,
										const std::string &time,
										const std::string &insurance_holder_id,
										const std::string &insurance_company_id,
										const std::string &insurance_company_name,
										const std::string &beneficiary_state,
										uint64_t loan_id);

	// @abi action
	void moddelinsure(uint64_t contract_id, const std::string &beneficiary, uint64_t loan_id);


private:
	// @abi table holderinfo i64
	struct holderinfo {
		uint64_t contract_id;
		uint64_t loan_id;
		std::string insurance_holder;
		std::string quantity;
		std::string insurance_num;
		std::string insurance_company_id;
		std::string insurance_company_name;
		std::string download_url;
		std::string time;
		std::string state;

		uint64_t primary_key() const { return contract_id; }

		EOSLIB_SERIALIZE(
				holderinfo, (contract_id)
				(loan_id)
				(insurance_holder)
				(quantity)
				(insurance_num)
				(insurance_company_id)
				(insurance_company_name)
				(download_url)
				(time)
				(state))
	};

	// @abi table beneficinfo
	struct beneficinfo {
		uint64_t beneficiary_id;
		uint64_t contract_id;
		uint64_t beneficiary_order_num;
		uint64_t loan_id;
		std::string beneficiary;
		std::string quantity;
		std::string weight;
		std::string insurance_num;
		std::string download_url;
		std::string time;
		std::string insurance_holder_id;
		std::string insurance_company_id;
		std::string insurance_company_name;
		std::string beneficiary_state;

		uint64_t primary_key() const { return beneficiary_id; }

		uint64_t get_contract_id() const { return contract_id; }

		EOSLIB_SERIALIZE(
				beneficinfo, (beneficiary_id)
				(contract_id)
				(loan_id)
				(beneficiary)
				(quantity)
				(weight)
				(insurance_num)
				(beneficiary_order_num)
				(download_url)
				(time)
				(insurance_holder_id)
				(insurance_company_id)
				(insurance_company_name)
				(beneficiary_state)
		)
	};

	typedef multi_index<N(holderinfo), holderinfo> holderTable;
	typedef multi_index<N(beneficinfo), beneficinfo,
			eosio::indexed_by<N(contractid),
					eosio::const_mem_fun<beneficinfo, uint64_t,
							&beneficinfo::get_contract_id>>> beneficTable;
};

#endif //EOSIO_INSURANCEPOLICY_H
