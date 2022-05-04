#include <farm.hpp>

ACTION farm::init(uint64_t reward_per_second) {
    require_auth(POOL_MANAGER);
    update_all_pools();

    global_t gl = _globals.get_or_create(get_self(), global_t{});
    gl.reward_per_second = reward_per_second;
    _globals.set(gl, _self);
}

ACTION farm::add(uint64_t pid, extended_symbol want, uint64_t weight, bool display) {
    require_auth(POOL_MANAGER);
    auto pool_itr = _pools.find(pid);
    if(pool_itr != _pools.end()) return;

    uint64_t now_time = current_time_point().sec_since_epoch();
    _pools.emplace(_self, [&](auto &a) {
        a.pid = pid;
        a.want = want;
        a.weight = weight;
        a.last_reward_time = now_time;
        a.reward_per_share = 0;
        a.display = display;
        a.enabled = true;
    });
}

ACTION farm::rmpool(uint64_t pid) {
    require_auth(POOL_MANAGER);
    auto itr = _pools.require_find(pid, "not fund pool");
    _pools.erase(itr);

    users _users(_self, pid);
    auto user_itr = _users.begin();
    while(user_itr != _users.end()) {
        _users.erase(user_itr);
        user_itr = _users.begin();
    }
}

void farm::clearuser(uint64_t pid, name owner) {
    require_auth(POOL_MANAGER);
    users _users(_self, pid);
    auto m_itr = _users.require_find(owner.value, "Pair does not exist.");
    _users.erase(m_itr);
}

ACTION farm::setrewardper(uint64_t reward_per_second) {
    require_auth(POOL_MANAGER);
    update_all_pools();

    global_t gl = _globals.get();
    gl.reward_per_second = reward_per_second;
    _globals.set(gl, _self);
}

ACTION farm::setweight(uint64_t pid, uint64_t weight) {
    require_auth(POOL_MANAGER);
    update_pool(pid);
    auto pool_itr = _pools.require_find(pid, "not fund pid");
    _pools.modify(pool_itr, same_payer, [&](auto &a) {
        a.weight = weight;
    });
}

ACTION farm::setdisplay(uint64_t pid, bool display) {
    require_auth(POOL_MANAGER);
    auto pool_itr = _pools.require_find(pid, "not fund pid");
    _pools.modify(pool_itr, same_payer, [&](auto &a) {
        a.display = display;
    });
}

ACTION farm::setenabled(uint64_t pid, bool enabled) {
    require_auth(POOL_MANAGER);
    update_pool(pid);
    uint64_t now_time = current_time_point().sec_since_epoch();
    auto pool_itr = _pools.require_find(pid, "not fund pid");
    _pools.modify(pool_itr, same_payer, [&](auto &a) {
        a.enabled = enabled;
        a.last_reward_time = now_time;
    });
}

void farm::update_all_pools() {
    auto itr = _pools.begin();
    while(itr != _pools.end()) {
        update_pool(itr->pid);
        itr++;
    }
}

void farm::update_pool(uint64_t pid) {
    auto pool_itr = _pools.find(pid);
    if(pool_itr == _pools.end()) return;
    if (!pool_itr->enabled) return;

    uint64_t now_time = current_time_point().sec_since_epoch();
    if (now_time <= pool_itr->last_reward_time) {
        return;
    }

    if (pool_itr->shares_total == 0) {
        _pools.modify(pool_itr, same_payer, [&](auto &a) {
            a.last_reward_time = now_time;
        });
        return;
    }

    uint64_t multiplier = now_time - pool_itr->last_reward_time;
    if (multiplier <= 0) {
        return;
    }

    global_t global = _globals.get();
    uint64_t reward = multiplier * global.reward_per_second * pool_itr->weight;
    if (reward == 0) {
        _pools.modify(pool_itr, same_payer, [&](auto &a) {
            a.last_reward_time = now_time;
        });
        return;
    }

    uint64_t reward_per_share = pool_itr->reward_per_share + reward/pool_itr->shares_total;
    _pools.modify(pool_itr, same_payer, [&](auto &a) {
        a.reward_per_share = reward_per_share;
        a.last_reward_time = now_time;
        a.total_rewards += reward;
    });

    asset rewards(reward, REWARD_SYMBOL);
    auto data = make_tuple(_self, _self, rewards, string("issue token"));
    action(permission_level{_self, "active"_n}, REWARD_CONTRACT, "mint"_n, data).send();
}

void farm::onlptokenchange(symbol_code code, uint64_t pid, name owner, uint64_t pre_amount, uint64_t now_amount) { 
    check(get_first_receiver() == SWAP_CONTRACT, "invalid contract code");
    require_auth(SWAP_CONTRACT);

    update_pool(pid);
    auto pool_itr = _pools.find(pid);
    if(pool_itr == _pools.end()) return;
    if(!pool_itr->enabled) return;

    string action = (now_amount - pre_amount) > 0 ? "deposit" : "withdraw";
    users _users(_self, pid);
    auto user_itr = _users.find(owner.value);
    if(action == "withdraw" && user_itr == _users.end()) return;

    uint64_t now_time = current_time_point().sec_since_epoch();
    if(pool_itr->shares_total == 0) {
        _pools.modify(pool_itr, same_payer, [&](auto &a) {
            a.reward_per_share = 0;
            a.total_rewards = 0;
            a.last_reward_time = now_time;
        });
    }
    
    if(user_itr == _users.end()) {
        _users.emplace(_self, [&](auto &a) {
            a.owner = owner;
            a.shares = now_amount;
            a.reward_debt = now_amount * pool_itr->reward_per_share;
            a.last_staked_time = now_time;
        });

        _pools.modify(pool_itr, same_payer, [&](auto &a) {
            a.shares_total += now_amount;
        });
        return;
    } else {
        _pools.modify(pool_itr, same_payer, [&](auto &a) {
            a.shares_total += now_amount - pre_amount;
        });
    }
    
    int128_t pending = user_itr->shares * pool_itr->reward_per_share - user_itr->reward_debt;
    if(now_amount == 0) {
        _users.erase(user_itr);
    } else {
        _users.modify(user_itr, same_payer, [&](auto &a) {
            a.shares = now_amount;
            a.reward_debt = now_amount * pool_itr->reward_per_share;
            if(action == "deposit") a.last_staked_time = now_time;
            if(action == "withdraw") a.last_withdraw_time = now_time;
        });
    }

    pool_itr = _pools.find(pid);
    if(pool_itr->shares_total == 0) {
        _pools.modify(pool_itr, same_payer, [&](auto &a) {
            a.reward_per_share = 0;
            a.total_rewards = 0;
            a.last_reward_time = now_time;
        });
    }

    if (pending > 0) {
        asset reward = asset(pending, REWARD_SYMBOL);
        utils::inline_transfer(REWARD_CONTRACT, _self, owner, reward, string("reward token"));
    }     
}

ACTION farm::claim(uint64_t pid, name owner) {
    require_auth(owner);
    update_pool(pid);

    users _users(_self, pid);
    auto pool_itr = _pools.require_find(pid, "not fund pid");
    auto user_itr = _users.require_find(owner.value, "not fund user");
    check(user_itr->shares > 0, "user shares is 0");

    uint64_t pending = user_itr->shares * pool_itr->reward_per_share - user_itr->reward_debt;
    if (pending <= 0) return;

    asset reward = asset(pending, REWARD_SYMBOL);
    if(reward.amount <= 0) return;
    _users.modify(user_itr, same_payer, [&](auto &a) {
        a.reward_debt = user_itr->shares * pool_itr->reward_per_share;
    });

    utils::inline_transfer(REWARD_CONTRACT, _self, owner, reward, string("reward token"));
}

