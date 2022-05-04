#include <utils.hpp>
#include <math.h>

#include <dfs.hpp>
#include <defibox.hpp>
#include <aiswap.hpp>

using namespace dfs;
using namespace defibox;
using namespace aiswap;

CONTRACT mswap : public contract
{
public:
   using contract::contract;

   static constexpr uint64_t PRICE_BASE = 10000;

   [[eosio::on_notify("*::transfer")]]
   void on_transfer( const name from, const name to, const asset quantity, const string memo );

   [[eosio::action]]
   void afterswap(const name owner, const asset in_quantity, extended_symbol out_sym);

private:
   void do_swap(const name from, const name to, const asset quantity, const string memo, name code);
   int128_t get_amount_out(int128_t amount_in, int128_t reserve_in, int128_t reserve_out);

   string get_swap(int i) {
      if (i == 1) return "defibox";
      if (i == 2) return "dfs";
      if (i == 3) return "aiswap";
      check(false, "invalid get_swap");
   }

   name get_swap_contract(string swap_name) {
      if (swap_name == "dfs") return name("defisswapcnt");
      if (swap_name == "aiswap") return name("eosaidaoswat");
      if (swap_name == "defibox") return name("swap.defi");
      check(false, "Invalid swap_name!");
   }

   string get_swap_memo(string swap_name, uint64_t mid) {
      if (swap_name == "dfs") return string("swap:"+to_string(mid)+":0:2");
      if (swap_name == "aiswap") return string("swap,0,"+to_string(mid));
      if (swap_name == "defibox") return string("swap,0,"+to_string(mid));
      check(false, "Invalid swap_name!");
   }

   std::pair<asset, asset> get_reserves(string swap_name, uint64_t mid, symbol sort) {
      if (swap_name == "dfs") return dfs::get_reserves( mid, sort );
      if (swap_name == "defibox") return defibox::get_reserves( mid, sort );
      if (swap_name == "aiswap") return aiswap::get_reserves( mid, sort );
   }

   extended_symbol get_out_extended_sym(string swap_name, uint64_t mid, symbol sym) {
      if (swap_name == "dfs") return dfs::get_out_extended_sym( mid, sym );
      if (swap_name == "defibox") return defibox::get_out_extended_sym( mid, sym );
      if (swap_name == "aiswap") return aiswap::get_out_extended_sym(mid, sym);
   }

   // if(defibox_mid != 0) out_sym = defibox::get_out_extended_sym(defibox_mid, quantity.symbol);
    // if(dfs_mid != 0) out_sym = dfs::get_out_extended_sym( dfs_mid, quantity.symbol );
    // if(ai_mid != 0) out_sym = dfs::get_out_extended_sym( ai_mid, quantity.symbol );
};