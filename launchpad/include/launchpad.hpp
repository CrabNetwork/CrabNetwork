#pragma once
#include <utils.hpp>
#include <safemath.hpp>
#include <eosio/singleton.hpp>
#include <swap.hpp>
#include <config.hpp>
#include <defibox.hpp>
using namespace eosio;

CONTRACT launchpad : public contract {
public:
   using contract::contract;
   static constexpr uint64_t DAY_PER_SECOND = 86400;
   static constexpr name EOS_CONTRACT = name("eosio.token");
   static constexpr symbol EOS_SYMBOL = symbol("EOS", 4);
   static constexpr extended_symbol EOS_EXSYM { EOS_SYMBOL, EOS_CONTRACT };
   asset PROTOCOL_FEE = asset(10000, EOS_SYMBOL);
   
   #if TEST
      static constexpr name LOCKED_CONTRACT = name("eoslockvants");
      static constexpr name FEE_CONTRACT = name("aidaoswapfet");
      static constexpr name POOL_MANAGER = name("wdogdeployer");
   #else
      static constexpr name LOCKED_CONTRACT = name("eoslockvants");
      static constexpr name FEE_CONTRACT = name("aidaoswapfet");
      static constexpr name POOL_MANAGER = name("wdogdeployer");
   #endif

   ACTION create(name owner, uint64_t swap_type, uint64_t pair_id, extended_symbol presale_token_sym, asset presale_rate,
    asset soft_cap, asset hard_cap, asset min_contribution, asset max_contribution, uint64_t swap_liquidity,
    asset swap_listing_rate, string logo_link, string website_link, string github_link, string twitter_link,
    string reddit_link, string telegram_link, string project_desc, string any_memo, uint64_t whitepair_id,
    uint32_t presale_start_time, uint32_t presale_end_time, uint32_t liquidity_unlock_time);
   ACTION claim(uint64_t pid, name owner);
   ACTION depositafter(uint64_t pid);
   ACTION withdraw(uint64_t pid, name owner);
   ACTION setwhitepair(uint64_t pair_id, asset min_asset, uint64_t lock_days);
   ACTION rmwhitepair(uint64_t id);
   ACTION logpresale(name owner, asset pay_amount, asset get_amount);
   ACTION clear(uint64_t id);

   [[eosio::on_notify("*::transfer")]]
   void on_transfer(name from, name to, asset quantity, string memo);
   [[eosio::on_notify("*::safetransfer")]]
   void on_safetransfer( name from, name to, asset quantity, string memo);
   void on_transfer_do(name from, name to, asset quantity, string memo);

   void ido(name owner, asset quantity, uint64_t presale_id);
   void do_deposit(name owner, const extended_asset value);

private:
   struct token {
      name contract;
      symbol symbol;
   };
   
   TABLE presale {
      uint64_t id;
      name owner; 
      uint64_t swap_type;
      uint64_t pair_id;
      extended_symbol presale_token_sym;
      asset presale_token_balance;
      asset presale_rate;
      asset presale_get_balance;
      asset claimed_presale_get;
      bool claimed;
      asset soft_cap; 
      asset hard_cap; 
      asset min_contribution; 
      asset max_contribution; 
      uint64_t swap_liquidity;
      uint64_t whitepair_id;
      asset swap_listing_rate; 
      string logo_link; 
      string website_link; 
      string github_link; 
      string twitter_link;
      string reddit_link; 
      string telegram_link; 
      string project_desc; 
      string any_memo;
      uint32_t presale_start_time; 
      uint32_t presale_end_time;
      uint32_t liquidity_unlock_time;
      extended_asset locked_lptoken_balance;
      asset presale_tokens;
      asset swap_listing_tokens;
      asset platform_fee;
      asset risk_fee;
      asset presale_token_total;

      uint64_t primary_key() const { return id; }

      uint64_t by_tokensymcode() const { return presale_token_sym.get_symbol().code().raw(); }
      uint64_t by_tokencontract() const { return presale_token_sym.get_contract().value; }
      uint64_t by_owner() const { return owner.value; }
      uint64_t by_swap_type() const { return swap_type; }
   };

   TABLE user {
      name owner;
      asset paid;
      asset tokens;
      bool claimed;
      uint64_t primary_key() const { return owner.value; }
   };

   TABLE balances_t {
      uint64_t id;
      extended_symbol sym;
      asset balance;

      uint64_t primary_key() const { return id; };
      checksum256 asset_id_hash() const { return utils::hash_asset_id(sym); };
   };

   TABLE pool_t {
      uint64_t id;
      extended_symbol token_sym; 
      asset locked_total;
      uint32_t locked_users;
      uint32_t last_locked_time;

      uint64_t primary_key() const { return id; }
      checksum256 hash_asset_id() const { return utils::hash_asset_id(token_sym); }
   };

   TABLE pair_t {
      uint64_t id;
      symbol_code lptoken_code;
      extended_symbol token0;
      extended_symbol token1;
      asset reserve0;
      asset reserve1;
      asset liquidity;
      double price0_last; // the last price is easy to controll, just for kline display, don't use in other dapp directly
      double price1_last;
      uint64_t price0_cumulative_last;
      uint64_t price1_cumulative_last;
      time_point_sec last_update;

      uint64_t primary_key() const { return id; }
      uint64_t lptoken_code_id() const { return lptoken_code.raw(); };
      checksum256 asset_ids_hash() const { return utils::hash_asset_ids(token0, token1); };
   };

   TABLE white_pair_t {
      uint64_t id;
      uint64_t pair_id;
      asset min_asset;
      uint64_t lock_days;

      uint64_t primary_key() const { return id; }
   };

   TABLE liquidity_t {
      name owner;
      asset amount0;
      asset amount1;
      asset token;
      uint64_t unlock_time;

      uint64_t primary_key() const { return owner.value; }
   };

   typedef eosio::multi_index<"presales1"_n, presale,
      indexed_by < name("bytksymcode"), const_mem_fun < presale, uint64_t, &presale::by_tokensymcode>>,
      indexed_by < name("bytkcontract"), const_mem_fun < presale, uint64_t, &presale::by_tokencontract>>,
      indexed_by < name("byowner"), const_mem_fun < presale, uint64_t, &presale::by_owner>>,
      indexed_by < name("byswaptype"), const_mem_fun < presale, uint64_t, &presale::by_swap_type>>> presales;
   
   typedef eosio::multi_index<"users1"_n, user> users;
   
   typedef multi_index <name("balances"), balances_t,
      indexed_by < name("assetidhash"), const_mem_fun < balances_t, checksum256, &balances_t::asset_id_hash>>> balances;
   
   typedef eosio::multi_index<"lockedpools"_n, pool_t,
      indexed_by < name("hashassetid"), const_mem_fun < pool_t, checksum256, &pool_t::hash_asset_id>>> pools;
   
   typedef multi_index<"pairs"_n, pair_t,
      indexed_by < name("lptokencode"), const_mem_fun < pair_t, uint64_t, &pair_t::lptoken_code_id>>,
      indexed_by < name("assetidshash"), const_mem_fun < pair_t, checksum256, &pair_t::asset_ids_hash>>> pairs;
   
   typedef multi_index<"liquidity2"_n, liquidity_t> liquiditys;
   
   typedef multi_index<"whitepairs"_n, white_pair_t> whitepairs;

   presales _presales = presales(_self, _self.value);
   pools _pools = pools(LOCKED_CONTRACT, LOCKED_CONTRACT.value);
   pairs _pairs = pairs(swap::code, swap::code.value);
   whitepairs _whitepairs = whitepairs(_self, _self.value);

   uint64_t get_locked_pool_id(extended_symbol ext_sym) {
      checksum256 hash_asset_id = utils::hash_asset_id(ext_sym);
      auto pool_by_hash = _pools.get_index<name("hashassetid")>();
      auto p_itr = pool_by_hash.require_find(hash_asset_id, "Lock Pool not fund!");
      return p_itr->id;
   }

   uint64_t get_pair_id(extended_symbol token0, extended_symbol token1) {
      if(!utils::is_eos(token0) && (utils::is_eos(token1) || utils::is_stable_token(token1))) {
        auto temp_token = token0;
        token0 = token1;
        token1 = temp_token;
      }

      checksum256 hash_asset_ids = utils::hash_asset_ids(token0, token1);
      auto pair_by_hash = _pairs.get_index<name("assetidshash")>();
      auto p_itr = pair_by_hash.find(hash_asset_ids);
      if(p_itr == pair_by_hash.end()) return 0;
      return p_itr->id;
   }

   name get_swap_contract(uint64_t swap_type) {
      if(swap_type == 0) return defibox::code; 
      if(swap_type == 1) return swap::code;
   }

   name get_lptoken_contract(uint64_t swap_type) {
      if(swap_type == 0) return defibox::lp_code;
      if(swap_type == 1) return swap::lp_code;
   }

   extended_symbol get_lptoken_from_pairid(uint64_t swap_type, uint64_t pair_id) {
      if(swap_type == 0) return defibox::get_lptoken_from_pairid(pair_id);
      if(swap_type == 1) return swap::get_lptoken_from_pairid(pair_id);
   }
};