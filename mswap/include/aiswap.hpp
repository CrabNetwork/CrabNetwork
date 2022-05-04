#pragma once

#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <utils.hpp>
#include <math.h>

namespace aiswap {

    using eosio::asset;
    using eosio::symbol;
    using eosio::name;
    using eosio::singleton;
    using eosio::multi_index;
    using eosio::time_point_sec;
    using eosio::current_time_point;
    using eosio::extended_symbol;
    using eosio::extended_asset;

    // reference
    const name id = "aiswap"_n;
    const name code = "eosaidaoswat"_n;
    const name lp_code = "swaplptokent"_n;
    const std::string lptoken_symcode_prefix = "CB";
    const std::string description = "AiSwap Converter";

    /**
     * Defibox pairs
     */
    struct [[eosio::table]] pairs_row {
        uint64_t id;
        symbol_code lptoken_code;
        extended_symbol token0;
        extended_symbol token1;
        asset reserve0;
        asset reserve1;
        asset liquidity;
        double price0_last; 
        double price1_last;
        uint64_t price0_cumulative_last;
        uint64_t price1_cumulative_last;
        time_point_sec last_update;

        uint64_t primary_key() const { return id; }
    };
    typedef eosio::multi_index< "pairs"_n, pairs_row > pairs;

    /**
     * Defibox stat
     */
    struct [[eosio::table("stat")]] stat_row {
        uint64_t            locked;
        uint64_t            staked;
        uint64_t            refunding;
    };
    typedef eosio::singleton< "stat"_n, stat_row > stat;

    /**
     * ## STATIC `get_reserves`
     *
     * Get reserves for a pair
     *
     * ### params
     *
     * - `{uint64_t} pair_id` - pair id
     * - `{symbol} sort` - sort by symbol (reserve0 will be first item in pair)
     *
     * ### returns
     *
     * - `{pair<asset, asset>}` - pair of reserve assets
     *
     * ### example
     *
     * ```c++
     * const uint64_t pair_id = 12;
     * const symbol sort = symbol{"EOS", 4};
     *
     * const auto [reserve0, reserve1] = defibox::get_reserves( pair_id, sort );
     * // reserve0 => "4585193.1234 EOS"
     * // reserve1 => "12568203.3533 USDT"
     * ```
     */
    static std::pair<asset, asset> get_reserves( const uint64_t pair_id, const symbol sort )
    {
        // table
        aiswap::pairs _pairs( code, code.value );
        auto pairs = _pairs.get( pair_id, "AiSwapLibrary: INVALID_PAIR_ID" );

        eosio::check( pairs.reserve0.symbol == sort || pairs.reserve1.symbol == sort, "AiSwapLibrary: sort symbol doesn't match");

        return sort == pairs.reserve0.symbol ?
            std::pair<asset, asset>{ pairs.reserve0, pairs.reserve1 } :
            std::pair<asset, asset>{ pairs.reserve1, pairs.reserve0 };
    }

    static extended_symbol get_out_extended_sym( const uint64_t pair_id, const symbol in_sym )
    {
        // table
        aiswap::pairs _pairs( code, code.value );
        auto pairs = _pairs.get( pair_id, "DefiboxLibrary: INVALID_PAIR_ID" );

        return in_sym == pairs.token0.get_symbol() ? extended_symbol{pairs.token1.get_symbol(), pairs.token1.get_contract()} : extended_symbol{pairs.token0.get_symbol(), pairs.token0.get_contract()};
    }

    /**
     * ## STATIC `get_pairid_from_lptoken`
     *
     * Get pair id from supplied BOX*** lp symbol code
     *
     * ### params
     *
     * - `{symbol_code} symcode` - BOX*** symbol code
     *
     * ### returns
     *
     * - `{uint64_t}` - defibox pair id
     *
     * ### example
     *
     * ```c++
     * const symbol_code symcode = symbolcode{"BOXGL"};
     *
     * const auto pair_id = defibox::get_pairid_from_lptoken( symcode );
     * // pair_id => 194
     * ```
     */
    static uint64_t get_pairid_from_lptoken( eosio::symbol_code lp_symcode )
    {
        std::string str = lp_symcode.to_string();
        if(str.length() < 3) return 0;
        uint64_t res = 0;
        if(str[0]!='B' || str[1]!='O' || str[2]!='X') return 0;
        for(auto i = 3; i < str.length(); i++){
            res *= 26;
            res += str[i] - 'A' + 1;
        }
        return res;
    }

    /**
     * ## STATIC `get_lptoken_from_pairid`
     *
     * Get LP token based on Defibox pair id
     *
     * ### params
     *
     * - `{uint64_t} pair_id` - Defibox pair id
     *
     * ### returns
     *
     * - `{extended_symbol}` - defibox lp token
     *
     * ### example
     *
     * ```c++
     * const uint64_t pair_id = 194;
     *
     * const auto ext_sym = defibox::get_lptoken_from_pairid( pair_id );
     * // ext_sym => "BOXGL,0"
     * ```
     */
    static extended_symbol get_lptoken_from_pairid( uint64_t pair_id )
    {
        if(pair_id == 0) return {};
        std::string res;
        while(pair_id){
            res = (char)('A' + pair_id % 26 - 1) + res;
            pair_id /= 26;
        }
        return { symbol { eosio::symbol_code{ lptoken_symcode_prefix + res }, 0 }, lp_code };
    }

    /**
     * ## STATIC `is_lptoken`
     *
     * Check if token is BOX*** LP token
     *
     * ### params
     *
     * - `{symbol} sym` - BOX*** symbol
     *
     * ### returns
     *
     * - `{bool}` - true if LP token, false if not
     *
     * ### example
     *
     * ```c++
     * const symbol sym = symbol{ {"BOXGL", 0} };
     *
     * const auto is = defibox::is_lptoken( sym );
     * // is => true
     * ```
     */
    static bool is_lptoken( const symbol& sym ) {
        return utils::get_supply({ sym, lp_code }).symbol.is_valid();
    }

}