#pragma once

#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <math.h>
#include "utils.hpp"
#include "config.hpp"

namespace crabswap {

    using eosio::asset;
    using eosio::symbol;
    using eosio::name;
    using eosio::singleton;
    using eosio::multi_index;
    using eosio::time_point_sec;
    using eosio::current_time_point;
    using eosio::extended_symbol;
    using eosio::extended_asset;

    #if TEST
        const name id = "wdogeswap"_n;
        const name code = "eosaidaoswat"_n;
        const name lp_code = "swaplptokent"_n;
   #else
        const name id = "wdogeswap"_n;
        const name code = "eosaidaoswat"_n;
        const name lp_code = "swaplptokent"_n;
   #endif
 
    symbol EOS_SYMBOL = symbol("EOS", 4);
    const std::string lptoken_symcode_prefix = "CB";
    const std::string description = "WdogeSwap Converter";

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
        double price0_last; // the last price is easy to controll, just for kline display, don't use in other dapp directly
        double price1_last;
        uint64_t price0_cumulative_last;
        uint64_t price1_cumulative_last;
        time_point_sec last_update;

        uint64_t primary_key() const { return id; }
        uint64_t lptoken_code_id() const { return lptoken_code.raw(); }
        checksum256 hash() const { return utils::hash_asset_ids(token0, token1); }
    };
    typedef multi_index<"pairs"_n, pairs_row,
      indexed_by < name("lptokencode"), const_mem_fun < pairs_row, uint64_t, &pairs_row::lptoken_code_id>>,
      indexed_by < name("byhash"), const_mem_fun < pairs_row, checksum256, &pairs_row::hash>>> pairs;

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
        crabswap::pairs _pairs( code, code.value );
        auto pairs = _pairs.get( pair_id, "AiSwapLibrary: INVALID_PAIR_ID" );

        eosio::check( pairs.reserve0.symbol == sort || pairs.reserve1.symbol == sort, "AiSwapLibrary: sort symbol doesn't match");

        return sort == pairs.reserve0.symbol ?
            std::pair<asset, asset>{ pairs.reserve0, pairs.reserve1 } :
            std::pair<asset, asset>{ pairs.reserve1, pairs.reserve0 };
    }

    static asset get_amount_out( const uint64_t pair_id, const asset in_asset, const uint16_t fee = 30 )
    {
        const auto [ reserve_in, reserve_out ] = crabswap::get_reserves( pair_id, in_asset.symbol );
        const uint64_t out = utils::get_amount_out_xxx( in_asset.amount, reserve_in.amount, reserve_out.amount, fee );
        return asset(out, reserve_out.symbol);
    }

    static asset get_user_eos( const uint64_t pair_id, uint64_t user_lptoken)
    {
        // table
        crabswap::pairs _pairs( code, code.value );
        auto pairs = _pairs.get( pair_id, "AiSwapLibrary: INVALID_PAIR_ID" );
        eosio::check( pairs.reserve0.symbol == EOS_SYMBOL || pairs.reserve1.symbol == EOS_SYMBOL, "SwapLibrary: sort symbol doesn't match");

        asset reserve_eos = EOS_SYMBOL == pairs.reserve0.symbol ?
            pairs.reserve0 : pairs.reserve1;
        asset user_eos = asset(reserve_eos.amount * user_lptoken * 2 / pairs.liquidity.amount, EOS_SYMBOL);
        return user_eos;
    }

    static std::pair<extended_symbol, extended_symbol> get_pair_extended_sym( const uint64_t pair_id, const symbol sort )
    {
        crabswap::pairs _pairs( code, code.value );
        auto pairs = _pairs.get( pair_id, "AiSwapLibrary: INVALID_PAIR_ID" );

        eosio::check( pairs.reserve0.symbol == sort || pairs.reserve1.symbol == sort, "AiSwapLibrary: sort symbol doesn't match");

        return sort == pairs.reserve0.symbol ?
            std::make_pair(pairs.token0, pairs.token1) :
            std::make_pair(pairs.token1, pairs.token0);
    }

    static extended_symbol get_out_extended_sym( const uint64_t pair_id, const symbol in_sym )
    {
        // table
        crabswap::pairs _pairs( code, code.value );
        auto pairs = _pairs.get( pair_id, "DefiboxLibrary: INVALID_PAIR_ID" );

        return in_sym == pairs.token0.get_symbol() ? extended_symbol{pairs.token1.get_symbol(), pairs.token1.get_contract()} : extended_symbol{pairs.token0.get_symbol(), pairs.token0.get_contract()};
    }

    static uint64_t get_pairid_by_tokens(extended_symbol token0, extended_symbol token1) {
        auto [tokenA, tokenB] = utils::sort_tokens(token0, token1);
        checksum256 hash_id = utils::hash_asset_ids(tokenA, tokenB);

        crabswap::pairs _pairs( code, code.value );
        auto pair_by_hash = _pairs.get_index<name("byhash")>();
        auto p_itr = pair_by_hash.find(hash_id);
        if(p_itr == pair_by_hash.end()) return 0;
        return p_itr->id;
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