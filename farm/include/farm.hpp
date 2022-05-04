#pragma once
#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include <eosio/singleton.hpp>
#include <math.h>
#include <string>
#include <utils.hpp>

using namespace std;
using namespace eosio;
using namespace utils;
using eosio::current_time_point;

CONTRACT farm : public contract {
public:
  using contract::contract;
  
  static constexpr name POOL_MANAGER = name("wdogdeployer");
  static constexpr name SWAP_CONTRACT = name("eosaidaoswat");
  static constexpr name REWARD_CONTRACT = name("aiaidaotoken");
  static constexpr symbol REWARD_SYMBOL = symbol("WDOGE", 3);
  static constexpr uint64_t POW_NUM = 1000;

  ACTION init(uint64_t reward_per_second);
  ACTION add(uint64_t pid, extended_symbol want, uint64_t weight, bool display);
  ACTION setrewardper(uint64_t reward_per_second);
  ACTION setweight(uint64_t pid, uint64_t weight);
  ACTION setdisplay(uint64_t pid, bool display);
  ACTION setenabled(uint64_t pid, bool enabled);
  // ACTION deposit(uint64_t pid, name owner, asset quantity);
  // ACTION withdraw(uint64_t pid, name owner, asset quantity);
  ACTION claim(uint64_t pid, name owner);
  ACTION clearuser(uint64_t pid, name owner);
  ACTION rmpool(uint64_t pid);

  [[eosio::on_notify("*::tokenchange")]]
  void onlptokenchange(symbol_code code, uint64_t pid, name owner, uint64_t pre_amount, uint64_t now_amount);

  // [[eosio::on_notify("*::lptokenchange")]]
  // void onlptoken_change(uint64_t pid, name owner, asset add_quantity, asset current_balance);

  // [[eosio::on_notify("*::logwithdraw")]]
  // void on_withdraw(uint64_t pid, name owner, asset sub_quantity, asset current_balance);

  // [[eosio::on_notify("*::transfer")]]
  // void on_transfer(name from, name to, asset quantity, std::string memo);
  // void on_deposit(name from, name to, asset quantity, std::string memo);

  // void logdeposit(uint64_t pid, name owner, asset want_amt, int128_t shares_added);
  // void logwithdraw(uint64_t pid, name owner, asset want_amt, int128_t shares_removed);

private:
  TABLE user_t {
    name owner;
    asset staked;
    uint64_t shares;
    uint64_t reward_debt;
    asset amount0;
    asset amount1;
    uint64_t last_staked_time;
    uint64_t last_withdraw_time;

    uint64_t primary_key() const { return owner.value; }
  }; 

  TABLE pool_t {
    uint64_t pid;
    extended_symbol want;
    uint64_t weight;
    uint64_t last_reward_time;
    uint64_t reward_per_share;
    uint64_t total_rewards;
    uint64_t shares_total;  
    asset total_staked;  
    bool display;
    bool enabled;
    
    uint64_t primary_key() const { return pid; }
  };

  struct [[eosio::table]] global_t {
      uint64_t reward_per_second;
  }; 

  typedef eosio::singleton<"globals"_n, global_t> globals;
  typedef multi_index<name("globals"), global_t> globals_for_abi;
  typedef multi_index<"users"_n, user_t> users;
  typedef multi_index<"pools"_n, pool_t> pools;

  pools _pools = pools(_self, _self.value);
  globals _globals = globals(_self, _self.value);

  void update_all_pools();
  void update_pool(uint64_t pid);
};
