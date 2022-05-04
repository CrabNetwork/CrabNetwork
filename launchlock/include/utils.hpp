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

using std::string;

bool token_exists(const name &token_contract_account, const symbol_code &sym_code) {
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

checksum256 hash_asset_id(extended_symbol token) {
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

asset get_supply( const extended_symbol ext_sym) {
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

extended_asset get_balance( const extended_symbol ext_sym, const name owner ) {
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

void inline_transfer(name contract, name from, name to, asset quantity, string memo) {
   action(
       permission_level{from, "active"_n},
       contract,
       name("transfer"),
       make_tuple(from, to, quantity, memo))
       .send();
}

vector<string> split( const string str, const string delim ) {
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

name parse_name(const string& str) {
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

symbol_code parse_symbol_code(const string& str) {
   if(str.size() > 7) return {};
   for (const auto c: str ) {
      if( c < 'A' || c > 'Z') return {};
   }
   const symbol_code sym_code = symbol_code{ str };

   return sym_code.is_valid() ? sym_code : symbol_code{};
}

bool is_digit( const string str ) {
   if ( !str.size() ) return false;
   for ( const auto c: str ) {
      if ( !isdigit(c) ) return false;
   }
   return true;
}

uint64_t get_sym_code_raw( const string sym_code ) {
   uint64_t result = 0;
   std::string_view str{ sym_code.c_str() };
    if( str.size() > 7 ) {
        check( false, "string is too long to be a valid symbol_code" );
    }
    for( auto itr = str.rbegin(); itr != str.rend(); ++itr ) {
        if( *itr < 'A' || *itr > 'Z') {
            check( false, "only uppercase letters allowed in symbol_code string" );
        }
        result <<= 8;
        result |= uint64_t(*itr);
    }
    return result;
}

uint8_t char_to_value( char c ) {
   if( c == '.')
      return 0;
   else if( c >= '1' && c <= '5' )
      return (c - '1') + 1;
   else if( c >= 'a' && c <= 'z' )
      return (c - 'a') + 6;
   else
      eosio::check( false, "character is not in allowed character set for names" );

   return 0; // control flow will never reach here; just added to suppress warning
}

int str_to_int(string s) {
   return atoi(s.c_str());
}

int64_t str_to_int64(string s) {
   return atoll(s.c_str());
}


bool is_stable_token( extended_symbol token ) {
   return (token.get_contract() == name("tethertether") && token.get_symbol() == symbol("USDT", 4)) ||
      (token.get_contract() == name("danchortoken") && token.get_symbol() == symbol("USN", 4));
}

bool is_eos( extended_symbol token ) {
   return token.get_contract() == name("eosio.token") && token.get_symbol() == symbol("EOS", 4);
}

} // namespace utils