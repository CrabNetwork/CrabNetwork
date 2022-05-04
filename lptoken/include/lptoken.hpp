#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include <eosio/singleton.hpp>

using std::string;
using namespace eosio;

class [[eosio::contract("lptoken")]] lptoken : public contract {
public:
   using contract::contract;
   static constexpr name SWAP_ACCOUNT = name("eosaidaoswat");
   
   [[eosio::action]] 
   void close(const name &owner, const symbol &symbol);
   [[eosio::action]] 
   void create(const asset &maximum_supply);
   [[eosio::action]] 
   void transfer(const name &from, const name &to, const asset &quantity, const string &memo);
   [[eosio::action]] 
   void mint(const name &to, const asset &quantity, const string &memo);
   [[eosio::action]] 
   void burn(const name &owner, const asset &quantity, const string &memo);
   [[eosio::action]] 
   void modify();
   [[eosio::action]] 
   void lock(name owner, symbol_code sym_code, uint64_t unlock_time);
   [[eosio::action]] 
   void unlock(name owner, symbol_code sym_code);

private:
   struct [[eosio::table]] account {
      asset balance;

      uint64_t primary_key() const { return balance.symbol.code().raw(); }
   };

   struct [[eosio::table]] currency_stats {
      asset supply;
      asset max_supply;
      name issuer;

      uint64_t primary_key() const { return supply.symbol.code().raw(); }
   };

   struct [[eosio::table]] minter {
      name minter;
      uint64_t primary_key() const { return minter.value; }
   };

   struct [[eosio::table]] lock_t {
      name owner;
      uint64_t unlock_time;

      uint64_t primary_key() const { return owner.value; }
   };

   typedef eosio::multi_index<"accounts"_n, account> accounts;
   typedef eosio::multi_index<"stat"_n, currency_stats> stats;
   typedef eosio::multi_index<"minters"_n, minter> minters;
   typedef eosio::multi_index<"locks"_n, lock_t> locks;

   minters _minters = minters(_self, _self.value);

   void sub_balance(const name &owner, const asset &value);
   void add_balance(const name &owner, const asset &value, const name &ram_payer);
};