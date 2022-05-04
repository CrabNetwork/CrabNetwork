#pragma once
#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include <eosio/singleton.hpp>
#include <math.h>
#include <string>
#include <utils.hpp>
#include <defilend.hpp>
#include <defibox.hpp>

using namespace std;
using namespace eosio;
using namespace utils;
using namespace defilend;
using namespace defibox;

using eosio::current_time_point;

CONTRACT boxlp : public contract {
public:
  using contract::contract;
  static constexpr name LEND_CONTRACT = name("lend.defi");
  static constexpr name BTOKEN_CONTRACT = name("btoken.defi");
  static constexpr name POOL_MANAGER = name("devpcrabfarm");
  static constexpr name CONTROL_FEE_ACC = name("controlfeess");
  static constexpr name BUYBACK_FEE_ACC = name("buybackfeess");
  static constexpr name PLATFORM_FEE_ACC = name("platformfees");

  struct strat_fee {
    uint64_t controller_fee;
    uint64_t platform_fee;
    uint64_t buyback_fee;
    uint64_t deposit_fee;
    uint64_t withdraw_fee;
  }; 

  struct fee_account {
    name control_fee_acc = CONTROL_FEE_ACC;
    name buyback_fee_acc = BUYBACK_FEE_ACC;
    name platform_fee_acc = PLATFORM_FEE_ACC;
  }; 

  ACTION init(
    extended_symbol want, 
    extended_symbol token0, 
    extended_symbol token1, 
    extended_symbol earn, 
    string earn_to_token0_path, 
    string earn_to_token1_path,
    name crab_farm_contract,
    strat_fee fees,
    uint64_t duration
  );

  ACTION update(
    extended_symbol want, 
    extended_symbol token0, 
    extended_symbol token1, 
    extended_symbol earn, 
    string earn_to_token0_path, 
    string earn_to_token1_path,
    name crab_farm_contract
  );

  ACTION setfees(strat_fee fees);
  ACTION setfeeacc(fee_account fee_accounts);
  ACTION setduration(uint64_t duration);
  ACTION setnexttime(uint64_t next_earn_time);
  ACTION depositback(uint64_t pid, name user_account, asset quantity);
  ACTION withdraw(uint64_t pid, name user_account, int128_t want_shares);
  ACTION earn();
  ACTION earnback();
  ACTION addliquidity();
  ACTION farm();
  ACTION logdeposit(uint64_t pid, name user_account, asset want_amt, int128_t shares_added);
  ACTION logwithdraw(uint64_t pid, name user_account, asset want_amt, int128_t shares_removed);
  ACTION withdraw2();

  [[eosio::on_notify("lptoken.defi::transfer")]]
  void ondeposit(name from, name to, asset quantity, std::string memo);
  
private:
  TABLE config_t {
    extended_symbol want;
    extended_symbol token0;
    extended_symbol token1;
    extended_symbol earn;
    name crab_farm_contract;
    string earn_to_token0_path;
    string earn_to_token1_path;
    strat_fee fees;
    fee_account fee_accounts;
    int128_t want_locked_total;
    int128_t shares_total;
    uint64_t last_earn_time;
    uint64_t next_earn_time;
    uint64_t duration;
  }; 

  typedef eosio::singleton<"configs21"_n, config_t> configs;
  typedef multi_index<name("configs21"), config_t> configs_for_abi;

  configs _configs = configs(_self, _self.value);
};
