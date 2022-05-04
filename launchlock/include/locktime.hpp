#pragma once
#include <utils.hpp>
#include <safemath.hpp>
#include <eosio/singleton.hpp>
#include <swap.hpp>
#include <defibox.hpp>
using namespace eosio;

#define TEST 1   

CONTRACT locktime : public contract {
public:
   using contract::contract;
   static constexpr uint64_t DAY_PER_SECOND = 86400;
   static constexpr name EOS_CONTRACT = name("eosio.token");
   static constexpr symbol EOS_SYMBOL = symbol("EOS", 4);
   static constexpr extended_symbol EOS_EXSYM { EOS_SYMBOL, EOS_CONTRACT };

   ACTION addlocktime(uint64_t pid, name owner, uint32_t day);
   ACTION withdraw(uint64_t pid, name owner);
   ACTION logdeposit(name owner, asset deposit_amount, name contract);
   ACTION logwithdraw(name owner, asset withdraw_amount, name contract);
   ACTION remove(uint64_t pid);

   [[eosio::on_notify("*::transfer")]]
   void on_transfer(name from, name to, asset quantity, string memo);
   void do_deposit(name owner, asset quantity, uint64_t presale_id, uint32_t unlock_time, name contract);

private:
   TABLE pool_t {
      uint64_t id;
      name owner;
      extended_symbol token_sym; 
      extended_symbol token0;
      extended_symbol token1;
      asset locked;
      bool is_lp;
      uint32_t unlock_time;

      uint64_t primary_key() const { return id; }
      uint64_t by_lpsymcode() const { return token_sym.get_symbol().code().raw(); }
      uint64_t by_tokensymcode() const { return token0.get_symbol().code().raw(); }
      uint64_t by_tokencontract() const { return token0.get_contract().value; }
      uint64_t by_owner() const { return owner.value; }
      uint64_t islp() const { return is_lp; }
   };

   typedef eosio::multi_index<"lockedpool"_n, pool_t,
      indexed_by < name("bylpsymcode"), const_mem_fun < pool_t, uint64_t, &pool_t::by_lpsymcode>>,
      indexed_by < name("bytksymcode"), const_mem_fun < pool_t, uint64_t, &pool_t::by_tokensymcode>>,
      indexed_by < name("bytkcontract"), const_mem_fun < pool_t, uint64_t, &pool_t::by_tokencontract>>,
      indexed_by < name("byowner"), const_mem_fun < pool_t, uint64_t, &pool_t::by_owner>>,
      indexed_by < name("islp"), const_mem_fun < pool_t, uint64_t, &pool_t::islp>>> pools;

   pools _pools = pools(_self, _self.value);
};