#include <locktime.hpp>
#include <math.h>

void locktime::addlocktime(uint64_t pid, name owner, uint32_t day) {
    require_auth(owner);

    auto p_itr = _pools.require_find(pid, "Presale not exists!");
    auto now_time = current_time_point().sec_since_epoch();
    _pools.modify(p_itr, same_payer, [&](auto &a) {
        a.unlock_time += day * 24 * 3600;
    }); 
}

void locktime::on_transfer(name from, name to, asset quantity, string memo) {
    require_auth( from );
    if ( to != get_self() || from == "eosio.ram"_n || from == "mine4.defi"_n) return;
    
    const vector<string> parts = utils::split(memo, ",");
    if(parts[0] != "deposit" && parts[0] != "deposit_launch") return;
    uint32_t unlock_time = stol(parts[1]); 
    if(parts[0] == "deposit") {   
        uint32_t pid = stol(parts[2]); 
        do_deposit(from, quantity, pid, unlock_time, get_first_receiver());
    } else if(parts[0] == "deposit_launch") {
        name owner = name(parts[2]);
        do_deposit(from, quantity, 0, unlock_time, get_first_receiver());
    }
}

// 质押锁定资金
void locktime::do_deposit(name owner, asset quantity, uint64_t pid, uint32_t unlock_time, name contract) {
    auto now_time = current_time_point().sec_since_epoch();
    check(unlock_time >= now_time + DAY_PER_SECOND*30, "Lock time must be at least 30 day after now time");
    auto ext_sym = extended_symbol{quantity.symbol, contract}; 
    
    bool is_lp = contract == swap::lp_code || contract == defibox::lp_code;
    extended_symbol token0; extended_symbol token1;
    if(contract == swap::lp_code) {
        uint64_t pair_id = swap::get_pairid_from_lptoken(quantity.symbol.code());
        const auto [sym0, sym1] = swap::get_pair_symbols(pair_id, EOS_SYMBOL);
        token0 = sym0; token1 = sym1;
    } else if(contract == defibox::lp_code) {
        uint64_t pair_id = defibox::get_pairid_from_lptoken(quantity.symbol.code());
        const auto [sym0, sym1] = defibox::get_pair_symbols(pair_id, EOS_SYMBOL);
        token0 = sym0; token1 = sym1;
    }

    auto p_itr = _pools.find(pid);
    if(p_itr == _pools.end()) {
        auto pid = _pools.available_primary_key();
        if (pid == 0) pid = 1;
        _pools.emplace(_self, [&](auto &a) {
            a.id = pid;
            a.owner = owner;
            a.token_sym = ext_sym; 
            a.token0 = token0;
            a.token1 = token1;
            a.locked = quantity;
            a.is_lp = is_lp;
            a.unlock_time = unlock_time;
        });
    } else {
        check(p_itr->token_sym == ext_sym, "Invalid token symbol");
        _pools.modify(p_itr, same_payer, [&](auto &a) {
            a.locked += quantity;
            a.is_lp = is_lp;
            a.unlock_time = unlock_time;
        }); 
    }

    auto data = make_tuple(owner, quantity, contract);
    action(permission_level{get_self(), name("active")}, get_self(), name("logdeposit"), data).send();
}

// 取回锁定资金
void locktime::withdraw(uint64_t pid, name owner) {
    require_auth(owner);

    auto p_itr = _pools.require_find(pid, "Pool not exists!");
    auto now_time = current_time_point().sec_since_epoch();    
    asset withdraw_amount = p_itr->locked;
    name contract = p_itr->token_sym.get_contract();
    _pools.erase(p_itr);

    utils::inline_transfer(contract, _self, owner, withdraw_amount, "Withdraw locked amount");
    auto data = make_tuple(owner, withdraw_amount, contract);
    action(permission_level{get_self(), name("active")}, get_self(), name("logwithdraw"), data).send();
}

void locktime::logdeposit(name owner, asset deposit_amount, name contract) {
    require_auth(get_self());
}

void locktime::logwithdraw(name owner, asset withdraw_amount, name contract) {
    require_auth(get_self());
}

void locktime::remove(uint64_t pid) {
    require_auth(get_self());

    auto p_itr = _pools.require_find(pid, "Pool not exists!");
    _pools.erase(p_itr);
}