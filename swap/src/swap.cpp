#include <swap.hpp>
#include "./actions.cpp"

namespace crab {

ACTION swap::init( const name lptoken_contract ) {
    require_auth( POOL_MANAGER );
    auto config = _configs.exists() ? _configs.get() : config_t{};

    check( is_account( config.lptoken_contract ), "init: `lptoken_contract` does not exist");
    if ( config.lptoken_contract.value ) check( config.lptoken_contract == lptoken_contract, "init: `lptoken_contract` cannot be modified once initialized");

    config.lptoken_contract = lptoken_contract;
    _configs.set( config, get_self() );
}

ACTION swap::setfee( uint8_t trade_fee, uint8_t protocol_fee, name fee_account ) {
    require_auth(POOL_MANAGER);

    check( _configs.exists(), ERROR_CONFIG_NOT_EXISTS );
    auto config = _configs.get();
    check( trade_fee <= MAX_TRADE_FEE, "setfee: `trade_fee` has exceeded maximum limit");
    check( protocol_fee <= MAX_PROTOCOL_FEE, "setfee: `protocol_fee` has exceeded maximum limit");
    check( is_account( fee_account ), "curve.sx::setfee: `fee_account` does not exist");

    config.trade_fee = trade_fee;
    config.protocol_fee = protocol_fee;
    config.fee_account = fee_account;
    _configs.set( config, get_self() );
}

ACTION swap::setnotifiers( const vector<name> log_notifiers ) {
    require_auth( POOL_MANAGER );
    for ( const name notifier : log_notifiers ) {
        check( is_account( notifier ), "setnotifiers: `notifier` does not exist");
    }

    check( _configs.exists(), ERROR_CONFIG_NOT_EXISTS );
    auto config = _configs.get();
    config.notifiers = log_notifiers;
    _configs.set( config, get_self() );
}

ACTION swap::setpairnotif(uint64_t pair_id, vector<name> pair_notifiers ) {
    require_auth( POOL_MANAGER );
    for ( const name notifier : pair_notifiers ) {
        check( is_account( notifier ), "setnotifiers: `notifier` does not exist");
    }

    auto itr = _pairnotifiers.find(pair_id);
    if (itr == _pairnotifiers.end()) {
        _pairnotifiers.emplace(get_self(), [&](auto &a) {
            a.pair_id = pair_id;
            a.notifiers = pair_notifiers;
        });
    } else {
        _pairnotifiers.modify(itr, same_payer, [&](auto &a) {
            a.pair_id = pair_id;
            a.notifiers = pair_notifiers;
        });
    }
}

ACTION swap::createpair(name creator, extended_symbol token0, extended_symbol token1) {
    require_auth(creator);
    check(token0 != token1, "invalid pair");
    auto token_pair = utils::sort_tokens(token0, token1);
    auto tokenA = token_pair.first;
    auto tokenB = token_pair.second;

    auto supply0 = utils::get_supply(tokenA);
    check(supply0.amount > 0, "invalid token0");
    check(supply0.symbol == tokenA.get_symbol(), "invalid symbol0");
    auto supply1 = utils::get_supply(tokenB);
    check(supply1.amount > 0, "invalid token1");
    check(supply1.symbol == tokenB.get_symbol(), "invalid symbol1");

    checksum256 asset_ids_hash = utils::hash_asset_ids(tokenA, tokenB);
    auto pairs_by_hash = _pairs.get_index<name("assetidshash")>();
    auto pairs_itr = pairs_by_hash.find(asset_ids_hash);
    check(pairs_itr == pairs_by_hash.end(), "pair already exists");

    auto create_lptoken = get_create_lptoken();
    auto pair_id = create_lptoken.first;
    auto liquidity_token_sym = create_lptoken.second;
    symbol_code lptoken_code = liquidity_token_sym.get_symbol().code();
    bool token_exists = utils::token_exists(LPTOKEN_CONTRACT, lptoken_code);
    if ( !token_exists ) create( liquidity_token_sym );

    _pairs.emplace(creator, [&](auto &a) {
        a.id = pair_id;
        a.lptoken_code = lptoken_code;
        a.token0 = tokenA;
        a.token1 = tokenB;
        a.reserve0.symbol = tokenA.get_symbol();
        a.reserve1.symbol = tokenB.get_symbol();
        a.liquidity.symbol = liquidity_token_sym.get_symbol();
        a.last_update = current_time_point();
    });
}

ACTION swap::removepair(uint64_t id) {
    require_auth(POOL_MANAGER);
    auto itr = _pairs.require_find(id, "Market does not exist.");
    _pairs.erase(itr);
}

ACTION swap::deposit(name owner, uint64_t pair_id) {
    require_auth(owner);
    add_liquidity(owner, pair_id);
}

ACTION swap::cancel(name owner) {
    require_auth(owner);
}

void swap::on_safetransfer( name from, name to, asset quantity, string memo) {
    name code = get_first_receiver();
    require_auth( code );
    on_transfer_do(from, to, quantity, memo, code);
}

void swap::on_transfer( name from, name to, asset quantity, string memo) {
    require_auth( from );
    name code = get_first_receiver();
    if(code == LPTOKEN_CONTRACT && from != get_self() && to != get_self()) {
        lptoken_change(from, to, quantity, memo);
    }

    on_transfer_do(from, to, quantity, memo, code);
}

void swap::lptoken_change(name from, name to, asset quantity, string memo) {
    uint64_t pair_id = utils::get_pairid_from_lptoken(quantity.symbol.code(), 2);
    auto m_itr = _pairs.require_find(pair_id, "Pair does not exist.");
    int128_t amount0 = quantity.amount * m_itr->reserve0.amount / m_itr->liquidity.amount;
    int128_t amount1 = quantity.amount * m_itr->reserve1.amount / m_itr->liquidity.amount;

    liquiditys liqtable(get_self(), pair_id);
    auto liq_itr = liqtable.require_find(from.value, "from does not exist.");
    bool isAllTransfer = liq_itr->token == quantity;
    if(liq_itr != liqtable.end()) {
        if(isAllTransfer) {
            liqtable.erase(liq_itr);
        } else {
            liqtable.modify(liq_itr, same_payer, [&](auto &a) {
                a.amount0.amount -= amount0;
                a.amount1.amount -= amount1;
                a.token -= quantity;
            });
        }
    }
    
    auto to_liq_itr = liqtable.find(to.value);
    if (to_liq_itr == liqtable.end()) {
        if(isAllTransfer) {
            liqtable.emplace(get_self(), [&](auto &a) {
                a.owner = to;
                a.amount0 = liq_itr->amount0;
                a.amount1 = liq_itr->amount1;
                a.token = quantity;
            });
        } else {
            liqtable.emplace(get_self(), [&](auto &a) {
                a.owner = to;
                a.amount0 = asset(amount0, m_itr->reserve0.symbol);
                a.amount1 = asset(amount1, m_itr->reserve1.symbol);
                a.token = quantity;
            });
        }
    } else {
        if(isAllTransfer) {
            liqtable.modify(to_liq_itr, same_payer, [&](auto &a) {
                a.amount0 += liq_itr->amount0;
                a.amount1 += liq_itr->amount1;
                a.token += quantity;
            });
        } else {
            liqtable.modify(to_liq_itr, same_payer, [&](auto &a) {
                a.amount0.amount += amount0;
                a.amount1.amount += amount1;
                a.token += quantity;
            });
        }
    }
}

void swap::on_transfer_do(name from, name to, asset quantity, string memo, name code) {
    if ( to != get_self() || from == "eosio.ram"_n || from == "mine4.defi"_n) return; 

    const auto parsed_memo = parse_memo( memo );
    const extended_asset ext_in = { quantity, code };
    if (parsed_memo.action == "deposit"_n) {
        do_deposit(from, parsed_memo.pair_ids[0], ext_in);
    } else if (parsed_memo.action == "withdraw"_n) {
        do_withdraw(from, parsed_memo.pair_ids[0], ext_in);
    } else if (parsed_memo.action == "swap"_n) {
        do_swap(from, ext_in, parsed_memo.pair_ids, parsed_memo.min_return);
    } 
}

void swap::do_swap(const name owner, const extended_asset ext_quantity, const vector<uint64_t> pair_ids, const int64_t min_return ) {
    extended_asset ext_out;
    extended_asset ext_in = ext_quantity;
    auto config = _configs.get();
    for ( const uint64_t pair_id : pair_ids ) {
        auto ext_in_sym = ext_in.get_extended_symbol();
        auto m_itr = _pairs.require_find(pair_id, "Pair does not exist.");
        check(ext_in_sym == m_itr->token0 || ext_in_sym == m_itr->token1, "Invalid symbol");
        
        const extended_asset protocol_fee = { ext_in.quantity.amount * config.protocol_fee / 10000, ext_in.get_extended_symbol() };
        uint64_t amount_in = ext_in.quantity.amount - protocol_fee.quantity.amount;
        
        asset new_reserve0; 
        asset new_reserve1;
        int128_t amount_out = 0;
        int128_t reserve0 = m_itr->reserve0.amount;
        int128_t reserve1 = m_itr->reserve1.amount;
        if (ext_in_sym == m_itr->token0) {
            amount_out = get_amount_out(amount_in, reserve0, reserve1);
            new_reserve0 = asset(reserve0 + amount_in, m_itr->token0.get_symbol());
            new_reserve1 = asset(reserve1 - amount_out, m_itr->token1.get_symbol());
            update(pair_id, new_reserve0.amount, new_reserve1.amount, reserve0, reserve1);
            ext_out = {static_cast<int64_t>(amount_out), m_itr->token1}; 
        } else {
            amount_out = get_amount_out(amount_in, reserve1, reserve0);
            new_reserve0 = asset(reserve0 - amount_out, m_itr->token0.get_symbol());
            new_reserve1 = asset(reserve1 + amount_in, m_itr->token1.get_symbol());
            update(pair_id, new_reserve0.amount, new_reserve1.amount, reserve0, reserve1);
            ext_out = {static_cast<int64_t>(amount_out), m_itr->token0};
        }

        if (protocol_fee.quantity.amount > 0) {
            utils::inline_transfer(ext_in_sym.get_contract(), get_self(), config.fee_account, protocol_fee.quantity, std::string("swap protocol fee"));
        }

        const double price = calculate_price( ext_in.quantity, ext_out.quantity );
        swap::swaplog_action swaplog( get_self(), { get_self(), "active"_n });
        swaplog.send( pair_id, owner, "swap"_n, ext_in.quantity, ext_out.quantity, protocol_fee.quantity, price, new_reserve0, new_reserve1 );

        ext_in = ext_out; 
    }

    check(ext_out.quantity.amount >= min_return, "INSUFFICIENT_OUTPUT_AMOUNT");
    if(ext_out.quantity.amount > 0) {
        auto ext_out_sym = ext_out.get_extended_symbol();
        utils::inline_transfer(ext_out_sym.get_contract(), get_self(), owner, ext_out.quantity, std::string("swap success"));
    }
}

int128_t swap::get_amount_out(int128_t amount_in, int128_t reserve_in, int128_t reserve_out) {
    auto config = _configs.get();
    check(amount_in > 0, "invalid input amount");
    check(reserve_in > 0 && reserve_out > 0, "insufficient liquidity");
    uint64_t amount_in_with_fee = amount_in * (PRICE_BASE - config.trade_fee);
    uint64_t numerator = amount_in_with_fee * reserve_out;
    uint64_t denominator = reserve_in * 10000 + amount_in_with_fee;
    uint64_t amount_out = numerator / denominator;
    check(amount_out > 0, "invalid output amount");
    return amount_out;
}

void swap::do_deposit( const name owner, const uint64_t pair_id, const extended_asset value ) {
    auto m_itr = _pairs.require_find(pair_id, "Pair does not exist.");
    auto ext_sym = value.get_extended_symbol();
    check(ext_sym == m_itr->token0 || ext_sym == m_itr->token1, "Invalid deposit.");

    balances _balances = balances(get_self(), owner.value);
    checksum256 asset_id_hash = utils::hash_asset_id(ext_sym);
    auto balances_by_hash = _balances.get_index<name("assetidhash")>();
    auto balance_itr = balances_by_hash.find(asset_id_hash);
    if(balance_itr == balances_by_hash.end()) {
        auto id = _balances.available_primary_key();
        if (id == 0) id = 1;
        _balances.emplace(get_self(), [&](auto &a) {
            a.id = id;
            a.sym = ext_sym;
            a.balance = value.quantity;
        });
    } else {
        balances_by_hash.modify(balance_itr, same_payer, [&](auto &a) {
            a.balance += value.quantity;
        });
    }
}

void swap::do_withdraw(const name owner, const uint64_t pair_id, const extended_asset value) {
    auto m_itr = _pairs.require_find(pair_id, "Market does not exist.");
    auto ext_sym = value.get_extended_symbol();
    check(ext_sym.get_contract() == LPTOKEN_CONTRACT, "Invalid deposit.");
    check(ext_sym.get_symbol() == m_itr->liquidity.symbol, "Invalid deposit.");

    liquiditys liqtable(get_self(), pair_id);
    auto liq_itr = liqtable.require_find(owner.value, "Not fund owner");
    uint64_t unlock_time = liq_itr->unlock_time;
    auto now_time = current_time_point().sec_since_epoch();
    check(unlock_time < now_time, "Now time must be >= Liquidity unlock time");

    int128_t reserve0 = m_itr->reserve0.amount;
    int128_t reserve1 = m_itr->reserve1.amount;
    int128_t amount0 = value.quantity.amount * reserve0 / m_itr->liquidity.amount;
    int128_t amount1 = value.quantity.amount * reserve1 / m_itr->liquidity.amount;
    check(amount0 > 0 && amount1 > 0, "INSUFFICIENT_LIQUIDITY_BURNED");
    asset amount0_quantity{static_cast<int64_t>(amount0), m_itr->token0.get_symbol()};
    asset amount1_quantity{static_cast<int64_t>(amount1), m_itr->token1.get_symbol()};
    auto [pre_amount, now_amount] = burn_liquidity_token(pair_id, owner, value.quantity, amount0_quantity, amount1_quantity);
    update(pair_id, reserve0 - amount0, reserve1 - amount1, reserve0, reserve1);
   
    utils::inline_transfer(m_itr->token0.get_contract(), get_self(), owner, amount0_quantity, std::string("withdraw token0 liquidity"));
    utils::inline_transfer(m_itr->token1.get_contract(), get_self(), owner, amount1_quantity, std::string("withdraw token1 liquidity"));   

    swap::liquiditylog_action liquiditylog( get_self(), { get_self(), "active"_n });
    liquiditylog.send( pair_id, owner, "withdraw"_n, value.quantity, -amount0_quantity, -amount1_quantity, m_itr->liquidity - value.quantity, m_itr->reserve0 - amount0_quantity, m_itr->reserve1 - amount1_quantity );

    swap::tokenchange_action lptokenchange( get_self(), { get_self(), "active"_n });
    lptokenchange.send( m_itr->lptoken_code, m_itr->id, owner, pre_amount, now_amount );

    if(unlock_time > 0) {
        auto data = make_tuple(owner, liq_itr->token.symbol.code());
        action(permission_level{_self, "active"_n}, LPTOKEN_CONTRACT, "unlock"_n, data).send();
    }
}

void swap::add_liquidity(name owner, uint64_t pair_id) {
    auto m_itr = _pairs.require_find(pair_id, "Pair does not exist.");
    balances _balances = balances(get_self(), owner.value);
    auto balances_by_hash = _balances.get_index<name("assetidhash")>();

    checksum256 token0_hash = utils::hash_asset_id(m_itr->token0);
    checksum256 token1_hash = utils::hash_asset_id(m_itr->token1);
    auto token0_itr = balances_by_hash.find(token0_hash);
    auto token1_itr = balances_by_hash.find(token1_hash);
    if(token0_itr == balances_by_hash.end() || token0_itr->balance.amount == 0) return;
    if(token1_itr == balances_by_hash.end() || token1_itr->balance.amount == 0) return;
    
    int128_t amount0 = 0;
    int128_t amount1 = 0;
    int128_t amount0_desired = token0_itr->balance.amount;
    int128_t amount1_desired = token1_itr->balance.amount;
    int128_t reserve0 = m_itr->reserve0.amount;
    int128_t reserve1 = m_itr->reserve1.amount;
    int128_t refund_amount0 = 0;
    int128_t refund_amount1 = 0;
    if (reserve0 == 0 && reserve1 == 0) {
        amount0 = amount0_desired;
        amount1 = amount1_desired;
    } else {
        uint64_t amount1_optimal = quote(amount0_desired, reserve0, reserve1);
        if (amount1_optimal <= amount1_desired) {
            amount0 = amount0_desired;
            amount1 = amount1_optimal;
            refund_amount1 = amount1_desired - amount1_optimal;
        } else {
            uint64_t amount0_optimal = quote(amount1_desired, reserve1, reserve0);
            check(amount0_optimal <= amount0_desired, "math error");
            amount0 = amount0_optimal;
            amount1 = amount1_desired;
            refund_amount0 = amount0_desired - amount0_optimal;
        }
    }

    if (refund_amount0 > 0)
        utils::inline_transfer(m_itr->token0.get_contract(), get_self(), owner, asset(refund_amount0, m_itr->token0.get_symbol()), std::string("extra deposit refund"));
    if (refund_amount1 > 0)
        utils::inline_transfer(m_itr->token1.get_contract(), get_self(), owner, asset(refund_amount1, m_itr->token1.get_symbol()), std::string("extra deposit refund"));

    int128_t token_mint = 0;
    int128_t total_liquidity_token = m_itr->liquidity.amount;
    if (total_liquidity_token == 0) {
        token_mint = sqrt(amount0 * amount1) - MINIMUM_LIQUIDITY;
        mint_liquidity_token(m_itr->id, MIN_LP_ACCOUNT, asset(MINIMUM_LIQUIDITY, m_itr->liquidity.symbol), asset(0, m_itr->token0.get_symbol()), asset(0, m_itr->token1.get_symbol())); // permanently lock the first MINIMUM_LIQUIDITY tokens
    } else {
        int128_t x = amount0 * total_liquidity_token / reserve0;
        int128_t y = amount1 * total_liquidity_token / reserve1;
        token_mint = std::min(x, y);
    }
    
    check(token_mint > 0, "INSUFFICIENT_LIQUIDITY_MINTED");
    asset mint_quantity{static_cast<int64_t>(token_mint), m_itr->liquidity.symbol};
    asset amount0_quantity{static_cast<int64_t>(amount0), m_itr->token0.get_symbol()};
    asset amount1_quantity{static_cast<int64_t>(amount1), m_itr->token1.get_symbol()};
    auto [ pre_amount, now_amount ] = mint_liquidity_token(m_itr->id, owner, mint_quantity, amount0_quantity, amount1_quantity);
    asset total_liquidity = mint_quantity + m_itr->liquidity;
    update(m_itr->id, reserve0 + amount0, reserve1 + amount1, reserve0, reserve1);
    balances_by_hash.erase(token0_itr);
    balances_by_hash.erase(token1_itr);

    swap::tokenchange_action lptokenchange( get_self(), { get_self(), "active"_n });
    lptokenchange.send( m_itr->lptoken_code, m_itr->id, owner, pre_amount, now_amount );
}

std::pair<uint64_t, uint64_t> swap::mint_liquidity_token(uint64_t pair_id, name to, asset quantity, asset amount0, asset amount1) {
    uint64_t pre_amount = 0;
    uint64_t now_amount = quantity.amount;
    liquiditys liqtable(get_self(), pair_id);
    auto liq_itr = liqtable.find(to.value);
    if (liq_itr == liqtable.end()) {
        liqtable.emplace(get_self(), [&](auto &a) {
            a.owner = to;
            a.amount0 = amount0;
            a.amount1 = amount1;
            a.token = quantity;
        });
    } else {
        pre_amount = liq_itr->token.amount;
        now_amount += pre_amount;
        liqtable.modify(liq_itr, same_payer, [&](auto &a) {
            a.amount0 += amount0;
            a.amount1 += amount1;
            a.token += quantity;
        });
    }

    auto m_itr = _pairs.require_find(pair_id, "Pair does not exist.");
    _pairs.modify(m_itr, same_payer, [&](auto &a) {
        a.liquidity += quantity;
    });

    auto data = make_tuple(to, quantity, std::string("mint liquidity token"));
    action(permission_level{_self, "active"_n}, LPTOKEN_CONTRACT, "mint"_n, data).send();

    return std::pair<uint64_t, uint64_t>{ pre_amount, now_amount };
}

void swap::lockliq(uint64_t pair_id, name owner, uint32_t day) {
    require_auth(owner);

    auto now_time = current_time_point().sec_since_epoch();
    liquiditys liqtable(get_self(), pair_id);
    auto liq_itr = liqtable.require_find(owner.value, "Not fund owner");
    uint64_t unlock_time = liq_itr->unlock_time;
    if (unlock_time == 0) {
        unlock_time = now_time + (day * 3600 * 24);
    } else {
        unlock_time += day * 3600 * 24;
    }

    liqtable.modify(liq_itr, same_payer, [&](auto &a) {
        a.unlock_time = unlock_time;
    });

    auto data = make_tuple(owner, liq_itr->token.symbol.code(), unlock_time);
    action(permission_level{_self, "active"_n}, LPTOKEN_CONTRACT, "lock"_n, data).send();
}

std::pair<uint64_t, uint64_t> swap::burn_liquidity_token(uint64_t pair_id, name to, asset quantity, asset amount0, asset amount1) {
    uint64_t pre_amount = 0;
    uint64_t now_amount = 0;
    liquiditys liqtable(get_self(), pair_id);
    auto liq_itr = liqtable.require_find(to.value, "User liquidity does not exist.");
    check(liq_itr->token.amount > 0, "Liquidity token is zero.");

    pre_amount = liq_itr->token.amount;
    if (liq_itr->token.amount - quantity.amount <= 0) {
        liqtable.erase(liq_itr);
    } else {
        now_amount = pre_amount - quantity.amount;
        liqtable.modify(liq_itr, same_payer, [&](auto &a) {
            a.token -= quantity;
            a.amount0.amount = a.amount0 <= amount0 ? 0 : a.amount0.amount - amount0.amount;
            a.amount1.amount = a.amount1 <= amount1 ? 0 : a.amount1.amount - amount1.amount;
            a.unlock_time = 0;
        });
    }

    auto m_itr = _pairs.require_find(pair_id, "Pair does not exist.");
    _pairs.modify(m_itr, same_payer, [&](auto &a) {
        a.liquidity -= quantity;
    });

    auto data = make_tuple(_self, quantity, std::string("burn liquidity token"));
    action(permission_level{_self, "active"_n}, LPTOKEN_CONTRACT, "burn"_n, data).send();

    return std::pair<uint64_t, uint64_t>{ pre_amount, now_amount };
}

void swap::update(uint64_t pair_id, int128_t balance0, int128_t balance1, int128_t reserve0, int128_t reserve1) {
    auto m_itr = _pairs.require_find(pair_id, "Pair does not exist.");
    auto last_sec = m_itr->last_update.sec_since_epoch();
    uint64_t time_elapsed = 1;
    if (last_sec > 0) time_elapsed = current_time_point().sec_since_epoch() - last_sec;
    _pairs.modify(m_itr, same_payer, [&](auto &a) {
        a.reserve0.amount = balance0;
        a.reserve1.amount = balance1;
        if (time_elapsed > 0 && reserve0 != 0 && reserve1 != 0){
            auto price0 = PRICE_BASE * reserve1 / reserve0;
            auto price1 = PRICE_BASE * reserve0 / reserve1;
            a.price0_cumulative_last += price0 * time_elapsed;
            a.price1_cumulative_last += price1 * time_elapsed;
            a.price0_last = (double)price0 / PRICE_BASE;
            a.price1_last = (double)price1 / PRICE_BASE;
        }
        a.last_update = current_time_point();
    });
}

// given some amount of an asset and pair reserves, returns an equivalent amount of the other asset
int128_t swap::quote(int128_t amount0, int128_t reserve0, int128_t reserve1) {
    check(amount0 > 0, "INSUFFICIENT_AMOUNT0");
    check(reserve0 > 0 && reserve1 > 0, "INSUFFICIENT_LIQUIDITY");
    return amount0 * reserve1 / reserve0;
}

memo_schema swap::parse_memo( const string memo ) {
    if(memo == "") return {};
    const vector<string> parts = utils::split(memo, ",");
    check(parts.size() <= 3, ERROR_INVALID_MEMO );

    memo_schema result;
    result.action = utils::parse_name(parts[0]);
    result.min_return = 0;
    if ( result.action == "swap"_n ) {
        result.pair_ids = parse_memo_pair_ids( parts[2] );
        check( utils::is_digit( parts[1] ), ERROR_INVALID_MEMO );
        result.min_return = std::stoll( parts[1] );
        check( result.min_return >= 0, ERROR_INVALID_MEMO );
        check( result.pair_ids.size() >= 1, ERROR_INVALID_MEMO );
    } else if ( result.action == "deposit"_n || result.action == "withdraw"_n ) {
        result.pair_ids = parse_memo_pair_ids( parts[1] );
        check( result.pair_ids.size() == 1, ERROR_INVALID_MEMO );
    }

    return result;
}

vector<uint64_t> swap::parse_memo_pair_ids( const string memo ) {
    set<uint64_t> duplicates;
    vector<uint64_t> pair_ids;
    for ( const string str : utils::split(memo, "-") ) {
        uint64_t pair_id = utils::str_to_int64( str );
        check( _pairs.find(pair_id) != _pairs.end(), "parse_memo_pair_ids: `pair_id` does not exist");
        pair_ids.push_back( pair_id );
        check( !duplicates.count( pair_id ), "parse_memo_pair_ids: invalid duplicate `pair_ids`");
        duplicates.insert( pair_id );
    }
    return pair_ids;
}

double swap::calculate_price( const asset value0, const asset value1 )
{
    const uint8_t precision_norm = max( value0.symbol.precision(), value1.symbol.precision() );
    const int64_t amount0 = mul_amount( value0.amount, precision_norm, value0.symbol.precision() );
    const int64_t amount1 = mul_amount( value1.amount, precision_norm, value1.symbol.precision() );
    return static_cast<double>(amount1) / amount0;
}

}
