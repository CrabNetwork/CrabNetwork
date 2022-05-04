#pragma once

#include <string>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/asset.hpp>
#include <eosio/system.hpp>
#include <eosio/crypto.hpp>

using namespace std;
using namespace eosio;

namespace utils
{
   using eosio::name;
   using eosio::asset;
   using eosio::symbol;
   using eosio::symbol_code;
   using eosio::extended_symbol;
   using eosio::extended_asset;
   using eosio::singleton;
   using eosio::multi_index;
   using eosio::time_point_sec;
   using eosio::current_time_point;

   using std::string;
   using std::vector;
   using std::pair;

static bool is_stable_token( extended_symbol token ) {
   return (token.get_contract() == name("tethertether") && token.get_symbol() == symbol("USDT", 4)) ||
      (token.get_contract() == name("danchortoken") && token.get_symbol() == symbol("USN", 4));
}

static bool is_eos( extended_symbol token ) {
   return token.get_contract() == name("eosio.token") && token.get_symbol() == symbol("EOS", 4);
}

static std::pair<extended_symbol, extended_symbol> sort_tokens( extended_symbol token0, extended_symbol token1 ) {
   // token0 => EOS|USDT|USN
   // token1 => TPT
   if(!is_eos(token1) && (is_eos(token0) || is_stable_token(token0))) {
      return std::make_pair(token1, token0);
   }

   // token0 => USDT|USN
   // token1 => EOS
   if(is_eos(token1) && is_stable_token(token0)) {
      return std::make_pair(token1, token0);
   }

   return std::make_pair(token0, token1);
}

static checksum256 hash_asset_id(extended_symbol token) {
   std::vector <uint64_t> asset_id = {
      token.get_contract().value,
      token.get_symbol().code().raw(),
      token.get_symbol().precision(),
   };

   uint64_t asset_id_array[asset_id.size()];
   std::copy(asset_id.begin(), asset_id.end(), asset_id_array);
   std::sort(asset_id_array, asset_id_array + asset_id.size());

   return eosio::sha256((char *) asset_id_array, sizeof(asset_id_array));
}

static checksum256 hash_asset_ids(extended_symbol token0, extended_symbol token1) {
   auto [tokenA, tokenB] = sort_tokens(token0, token1);
   std::vector <uint64_t> asset_ids = {
      tokenA.get_contract().value,
      tokenA.get_symbol().code().raw(),
      tokenA.get_symbol().precision(),
      tokenB.get_contract().value,
      tokenB.get_symbol().code().raw(),
      tokenB.get_symbol().precision()
   };

   uint64_t asset_ids_array[asset_ids.size()];
   std::copy(asset_ids.begin(), asset_ids.end(), asset_ids_array);
   std::sort(asset_ids_array, asset_ids_array + asset_ids.size());

   return eosio::sha256((char *) asset_ids_array, sizeof(asset_ids_array));
}

static bool token_exists(const name &token_contract_account, const symbol_code &sym_code) {
   struct [[eosio::table]] currency_stats {
      asset    supply;
      asset    max_supply;
      name     issuer;

      uint64_t primary_key()const { return supply.symbol.code().raw(); }
   };
   typedef eosio::multi_index< "stat"_n, currency_stats > stats;
   
   stats _stats(token_contract_account, sym_code.raw());
   auto stats_itr = _stats.find(sym_code.raw());

   return !(stats_itr == _stats.end());
}

static asset get_supply( const extended_symbol ext_sym) {
   struct [[eosio::table]] currency_stats {
      asset    supply;
      asset    max_supply;
      name     issuer;

      uint64_t primary_key()const { return supply.symbol.code().raw(); }
   };
   typedef eosio::multi_index< "stat"_n, currency_stats > stats;

   stats _stat(ext_sym.get_contract(), ext_sym.get_symbol().code().raw() );
   const auto it = _stat.begin();

   return it != _stat.end() ? it->supply : asset {};
}

static extended_asset get_balance( const extended_symbol ext_sym, const name owner ) {
   //eosio.token accounts table - private in eosio.token contract
   struct [[eosio::table]] account {
      asset    balance;
      uint64_t primary_key()const { return balance.symbol.code().raw(); }
   };
   typedef eosio::multi_index< "accounts"_n, account > accounts_table;

   accounts_table _accounts( ext_sym.get_contract(), owner.value );
   auto it = _accounts.find( ext_sym.get_symbol().code().raw() );
   if(it == _accounts.end()) return { 0, ext_sym };
   eosio::check( ext_sym.get_symbol() == it->balance.symbol, "SX.Utils: extended symbol mismatch balance");

   return { it->balance.amount, ext_sym };
}

static void inline_transfer(name contract, name from, name to, asset quantity, string memo) {
   action(
       permission_level{from, "active"_n},
       contract,
       name("transfer"),
       make_tuple(from, to, quantity, memo))
       .send();
}

static vector<string> split( const string str, const string delim ) {
   vector<string> tokens;
   if ( str.size() == 0 ) return tokens;

   size_t prev = 0, pos = 0;
   do
   {
      pos = str.find(delim, prev);
      if (pos == string::npos) pos = str.length();
      string token = str.substr(prev, pos-prev);
      if (token.length() > 0) tokens.push_back(token);
      prev = pos + delim.length();
   }
   while (pos < str.length() && prev < str.length());
   return tokens;
}

static name parse_name(const string& str) {
   if(str.length() == 0 || str.length() > 12) return {};
   int i=0;
   for(const auto c: str) {
      if((c >= 'a' && c <= 'z') || ( c >= '0' && c <= '5') || c == '.') {
            if(i == 0 && ( c >= '0' && c <= '5') ) return {};   //can't start with a digit
            if(i == 11 && c == '.') return {};                  //can't end with a .
      }
      else return {};
      i++;
   }
   return name{str};
}

static symbol_code parse_symbol_code(const string& str) {
   if(str.size() > 7) return {};
   for (const auto c: str ) {
      if( c < 'A' || c > 'Z') return {};
   }
   const symbol_code sym_code = symbol_code{ str };

   return sym_code.is_valid() ? sym_code : symbol_code{};
}

static bool is_digit( const string str ) {
   if ( !str.size() ) return false;
   for ( const auto c: str ) {
      if ( !isdigit(c) ) return false;
   }
   return true;
}

static uint64_t get_amount_out_xxx( const uint64_t amount_in, const uint64_t reserve_in, const uint64_t reserve_out, const uint16_t fee = 30 )
{
   //eosio::check(amount_in > 0, "SX.Uniswap: INSUFFICIENT_INPUT_AMOUNT");
   //eosio::check(reserve_in > 0 && reserve_out > 0, "SX.Uniswap: INSUFFICIENT_LIQUIDITY");
   if(amount_in == 0) return 0;
   if(reserve_in == 0 || reserve_out == 0) return 0;

   // calculations
   const uint128_t amount_in_with_fee = static_cast<uint128_t>(amount_in) * (10000 - fee);
   const uint128_t numerator = amount_in_with_fee * reserve_out;
   const uint128_t denominator = (static_cast<uint128_t>(reserve_in) * 10000) + amount_in_with_fee;
   const uint64_t amount_out = numerator / denominator;

   return amount_out;
}

bool valid_pair_id( uint64_t pair_id ) {
   std::string res;
   while(pair_id){
      res = (char)('A' + pair_id % 26 - 1) + res;
      pair_id /= 26;
   }

   bool valid = true;
   std::string_view str = res;
   for( auto itr = str.rbegin(); itr != str.rend(); ++itr ) {
      if( *itr < 'A' || *itr > 'Z') {
         valid = false;
         break;
      }
   }

   return valid;
}

static extended_symbol get_lptoken_from_pairid( uint64_t pair_id, name code, string prefix ) {
   if(pair_id == 0) return {};
   std::string res;
   while(pair_id){
      res = (char)('A' + pair_id % 26 - 1) + res;
      pair_id /= 26;
   }

   return { symbol { eosio::symbol_code{ prefix + res }, 0 }, code };
}

static uint64_t get_pairid_from_lptoken( eosio::symbol_code lp_symcode, int start ) {
   std::string str = lp_symcode.to_string();
   if(str.length() < 3) return 0;
   uint64_t res = 0;
   for(auto i = start; i < str.length(); i++){
      res *= 26;
      res += str[i] - 'A' + 1;
   }
   return res;
}

static int str_to_int(string s) {
   return atoi(s.c_str());
}

static int64_t str_to_int64(string s) {
   return atoll(s.c_str());
}

static uint128_t uint128_hash(const string& hash) {
    return std::hash<string>{}(hash);
}

} // namespace utils