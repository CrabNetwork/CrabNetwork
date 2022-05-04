#pragma once

#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include <eosio/asset.hpp>

namespace dfs {

    using eosio::asset;
    using eosio::symbol;
    using eosio::name;
    using eosio::time_point_sec;

    const name id = "dfs"_n;
    const name code = "defisswapcnt"_n;
    const std::string description = "DFS Converter";

    const uint64_t _lucky_time_gap = 5;         //lucky egg time gap in minutes
    const uint64_t _lucky_seconds = 10;         //consider first {_lucky_seconds} seconds every {_lucky_time_gap} minutes as lucky
    static bool _lucky_egg = false;             //set to true when we found lucky egg

    /**
     * DFS markets
     */
    struct [[eosio::table]] markets_row {
        uint64_t            mid;
        name                contract0;
        name                contract1;
        symbol              sym0;
        symbol              sym1;
        asset               reserve0;
        asset               reserve1;
        uint64_t            liquidity_token;
        double_t            price0_last;

        uint64_t primary_key() const { return mid; }
    };
    typedef eosio::multi_index< "markets"_n, markets_row > markets;

    /**
     * DFS egg args
     */
    struct [[eosio::table]] eggargs_row {
        uint64_t            mid;
        uint64_t            time_gap;
        double_t            lucky_discount;
        asset               trigger_value_max;

        uint64_t primary_key() const { return mid; }
    };
    typedef eosio::multi_index< "eggargs"_n, eggargs_row > eggargs;

    /**
     * DFS votes
     */
    struct [[eosio::table]] poolsvotes_row {
        uint64_t            mid;
        uint64_t            rank;
        double_t            total_votes;

        uint64_t primary_key() const { return mid; }
    };
    typedef eosio::multi_index< "pools"_n, poolsvotes_row > pools;

    /**
     * DFS eggs
     */
    struct [[eosio::table]] eggs_row {
        uint64_t            key;
        name                owner;
        asset               trade_value;
        time_point_sec      time;

        uint64_t primary_key() const { return key; }
    };
    typedef eosio::multi_index< "eggs"_n, eggs_row > eggs;

    // /**
    //  * DFS mining
    //  */
    // struct [[eosio::table]] poolslots_row {
    //     uint64_t            rank;
    //     double_t            default_distount;
    //     double_t            lucky_distount;
    //     double_t            pool_weight;
    //     asset               trigger_value_max;
    //     asset               daily_max_supply;
    //     asset               daily_supply;
    //     double_t            aprs;
    //     double_t            dynamic_aprs;
    //     time_point_sec      last_update;

    //     uint64_t primary_key() const { return rank; }
    // };
    // typedef eosio::multi_index< "poolslots"_n, poolslots_row > poolslots;

    /**
     * ## STATIC `get_fee`
     *
     * Get total fee
     *
     * ### params
     *
     * - `{name} [code="defisswapcnt"]` - code account
     *
     * ### returns
     *
     * - `{uint8_t}` - total fee (trade + protocol)
     *
     * ### example
     *
     * ```c++
     * const uint8_t fee = dfs::get_fee();
     * // => 30
     * ```
     */
    static uint8_t get_fee( const name code = dfs::code )
    {
        return 30;
    }

    /**
     * ## STATIC `get_reserves`
     *
     * Get reserves for a pair
     *
     * ### params
     *
     * - `{uint64_t} mid` - market id
     * - `{symbol} sort` - sort by symbol (reserve0 will be first item in pair)
     * - `{name} [code="defisswapcnt"]` - code account
     *
     * ### returns
     *
     * - `{pair<asset, asset>}` - pair of reserve assets
     *
     * ### example
     *
     * ```c++
     * const uint64_t mid = 17;
     * const symbol sort = symbol{"EOS", 4};
     *
     * const auto [reserve0, reserve1] = dfs::get_reserves( mid, sort );
     * // reserve0 => "4585193.1234 EOS"
     * // reserve1 => "12568203.3533 USDT"
     * ```
     */
    static std::pair<asset, asset> get_reserves( const uint64_t mid, const symbol sort, const name code = dfs::code )
    {
        // table
        dfs::markets _pairs( code, code.value );
        auto pairs = _pairs.get( mid, "DFSLibrary: INVALID_MID" );
        //eosio::check( pairs.reserve0.symbol == sort || pairs.reserve1.symbol == sort, "DFSLibrary: sort symbol "+sort.code().to_string()+" for pair "+to_string(mid)+" does not match reserves: "+pairs.reserve0.symbol.code().to_string()+","+pairs.reserve1.symbol.code().to_string());
        eosio::check( pairs.reserve0.symbol == sort || pairs.reserve1.symbol == sort, "DFSLibrary: sort symbol doesn't match");
        return sort == pairs.reserve0.symbol ?
            std::pair<asset, asset>{ pairs.reserve0, pairs.reserve1 } :
            std::pair<asset, asset>{ pairs.reserve1, pairs.reserve0 };
    }

    static extended_symbol get_out_extended_sym( const uint64_t mid, const symbol in_sym, const name code = dfs::code )
    {
        // table
        dfs::markets _pairs( code, code.value );
        auto pairs = _pairs.get( mid, "DFSLibrary: INVALID_MID" );
        return in_sym == pairs.sym0 ? extended_symbol{pairs.sym1, pairs.contract1} : extended_symbol{pairs.sym0, pairs.contract0};
    }

    //lucky egg times - first 5 seconds of every 10 minutes
    static bool is_lucky_time() {
        auto now = eosio::current_time_point().sec_since_epoch();
        return (now % (_lucky_time_gap * 60)) / _lucky_seconds == 0;
    }

    /**
     * ## STATIC `get_rewards`
     *
     * Get rewards for trading
     *
     * ### params
     *
     * - `{uint64_t} pair_id` - pair id
     * - `{asset} in` - input quantity
     * - `{asset} out` - out quantity
     * - `{name} [code="defisswapcnt"]` - code account
     *
     * ### returns
     *
     * - {asset} = rewards in DFS
     *
     * ### example
     *
     * ```c++
     * const uint64_t pair_id = 12;
     * const asset in = asset{10000, {"EOS", 4}};
     * const asset out = asset{12345, {"USDT", 4}};
     *
     * const auto rewards = dfs::get_rewards( pair_id, in, out );
     * // rewards => "0.123456 DFS"
     * ```
     */

    static asset get_rewards( const uint64_t pair_id, asset in, asset out, const name code = dfs::code )
    {
        asset reward {0, symbol{"DFS",4}};
        if (in.symbol != symbol{"EOS",4}) return reward;     //rewards only if EOS - incoming currency

        //check if pool is ranked
        dfs::pools _pools( "dfspoolsvote"_n, "dfspoolsvote"_n.value );
        auto poolit = _pools.find( pair_id );
        if(poolit==_pools.end() || poolit->rank==0 || poolit->rank>20) return reward;

        //default reward fee discount for ranked pools
        float discount = 0.2;
        if (is_lucky_time()) {

            dfs::eggargs _eggargs ("miningpool11"_n, "miningpool11"_n.value );
            auto rowit = _eggargs.find(pair_id);
            if(rowit != _eggargs.end() && in <= rowit->trigger_value_max) {

                dfs::eggs _eggs ("miningpool11"_n, rowit->mid );   //check if the egg was already taken
                auto last_egg = _eggs.rbegin();
                if(last_egg != _eggs.rend() && eosio::current_time_point().sec_since_epoch() - last_egg->time.sec_since_epoch() > _lucky_seconds ) {
                    discount = rowit->lucky_discount;
                    _lucky_egg = true;
                }
            }
        }

        dfs::markets _pairs( code, code.value );
        auto dfsrate = _pairs.get( 39, "DFSLibrary: Bad EOS/DFS market id" ).price0_last;

        // formula: https://github.com/defis-net/defis-network#mining-defis-network-mining-pools
        float fee = in.amount * get_fee() / 10000;
        reward.amount = fee * dfsrate * discount * 0.8 * 1.04;  //extra 4% for invite code

        return reward;
    }
}