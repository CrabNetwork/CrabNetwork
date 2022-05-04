#include <launchpad.hpp>
#include <math.h>

void launchpad::create(name owner, uint64_t swap_type, uint64_t pair_id, extended_symbol presale_token_sym, asset presale_rate,
    asset soft_cap, asset hard_cap, asset min_contribution, asset max_contribution, uint64_t swap_liquidity,
    asset swap_listing_rate, string logo_link, string website_link, string github_link, string twitter_link,
    string reddit_link, string telegram_link, string project_desc, string any_memo, uint64_t whitepair_id,
    uint32_t presale_start_time, uint32_t presale_end_time, uint32_t liquidity_unlock_time) {
    require_auth(owner);

    symbol sym = presale_token_sym.get_symbol();
    if(swap_type == 0) check(pair_id != 0, "pair_id not exists!");
    check(sym == presale_rate.symbol, "Invalid symbol");
    check(sym == swap_listing_rate.symbol, "Invalid symbol");
    check(EOS_SYMBOL == soft_cap.symbol, "Invalid symbol");
    check(EOS_SYMBOL == hard_cap.symbol, "Invalid symbol");
    check(EOS_SYMBOL == min_contribution.symbol, "Invalid symbol");
    check(EOS_SYMBOL == max_contribution.symbol, "Invalid symbol");
    check(soft_cap.amount > 0 && hard_cap.amount > 0, "Softcap and hadcap must be > 0");
    check(soft_cap.amount >= hard_cap.amount/2, "Softcap must be >= 50% of Hardcap");
    check(soft_cap > PROTOCOL_FEE * 10, "Presale softcap amount too small, softcap must be >= 10 EOS");
    check(min_contribution.amount > 0 && max_contribution.amount > 0, "Minimum and maximum contribution amounts must be > 0");
    check(max_contribution.amount >= min_contribution.amount, "maximum contribution must be > minimum contribution");
    check(swap_liquidity >= 51 && swap_liquidity <= 100, "Liquidity on Swap Min 51%, Max 100%, We recommend > 70%");
    check(presale_end_time - presale_start_time >= DAY_PER_SECOND*3, "Presale end time can not be less than 3 day from presale start time!");
    check(presale_end_time - presale_start_time <= DAY_PER_SECOND*7, "Presale end time can not be longer than 1 week from presale start time!");
    check(liquidity_unlock_time - presale_end_time >= DAY_PER_SECOND*90, "Liquidity Lock time must be at least 3 month after Presale End Time!");
    check(_whitepairs.find(whitepair_id) != _whitepairs.end(), "whitepair_id does not exist!");

    balances _balances = balances(get_self(), owner.value);
    checksum256 asset_id_hash = utils::hash_asset_id(presale_token_sym);
    auto balances_by_hash = _balances.get_index<name("assetidhash")>();
    auto balance_itr = balances_by_hash.find(asset_id_hash);
    check(balance_itr != balances_by_hash.end(), "Please transfer presale tokens to launchpad contract");

    // 预售所需代币
    auto presale_tokens = asset(hard_cap.amount * presale_rate.amount / 10000, sym);
    // 交易所上市所需代币
    auto swap_listing_tokens = asset(swap_liquidity * hard_cap.amount * swap_listing_rate.amount / 1000000, sym);
    // 平台费用
    auto platform_fee = asset(presale_tokens.amount * 2 / 100, sym);
    // 风险费用
    auto risk_fee = asset((platform_fee.amount + swap_listing_tokens.amount) * 2 / 100, sym);
    // 总数量
    auto presale_token_total = presale_tokens + swap_listing_tokens + platform_fee + risk_fee;
    check(balance_itr->balance >= presale_token_total, "Presale token balance not enough");
    balances_by_hash.erase(balance_itr);

    if(pair_id > 0) {
        const auto [ reserve_in, reserve_out ] = defibox::get_reserves(pair_id, EOS_SYMBOL);
        check(reserve_out.symbol == sym, "Pair ID Invalid!");
    }

    if(swap_type == 1) pair_id = get_pair_id(presale_token_sym, EOS_EXSYM);
    auto id = _presales.available_primary_key();
    if (id == 0) id = 1;
    _presales.emplace(owner, [&](auto &a) {
        a.id = id;
        a.owner = owner;
        a.swap_type = swap_type;
        a.pair_id = pair_id;
        a.whitepair_id = whitepair_id;
        a.presale_token_sym = presale_token_sym;
        // 预售所需代币
        a.presale_tokens = presale_tokens;
        // 交易所上市所需代币
        a.swap_listing_tokens = swap_listing_tokens;
        // 平台费用
        a.platform_fee = platform_fee;
        // 风险费用
        a.risk_fee = risk_fee;
        // 总数量
        a.presale_token_total = presale_token_total;
        a.presale_get_balance = asset(0, EOS_SYMBOL);
        a.presale_rate = presale_rate;  
        a.soft_cap = soft_cap;
        a.hard_cap = hard_cap;
        a.min_contribution = min_contribution;
        a.max_contribution = max_contribution;
        a.swap_liquidity = swap_liquidity;
        a.swap_listing_rate = swap_listing_rate;
        a.logo_link = logo_link;
        a.website_link = website_link;
        a.github_link = github_link;
        a.twitter_link = twitter_link;
        a.reddit_link = reddit_link;
        a.telegram_link = telegram_link;
        a.project_desc = project_desc;
        a.any_memo = any_memo;
        a.presale_start_time = presale_start_time;
        a.presale_end_time = presale_end_time;
        a.liquidity_unlock_time = liquidity_unlock_time;
    });

    name swap_contract = get_swap_contract(swap_type);
    if(swap_type == 0) {
        auto token0 = token{EOS_CONTRACT, EOS_SYMBOL};
        auto token1 = token{presale_token_sym.get_contract(), sym};
        action{permission_level{get_self(), "active"_n}, swap_contract, "createpair"_n, make_tuple(get_self(), token0, token1)}.send();
    } else if(swap_type == 1 && pair_id == 0) {
        action{permission_level{get_self(), "active"_n}, swap_contract, "newpair"_n, make_tuple(get_self(), presale_token_sym, EOS_EXSYM)}.send();
    }

    utils::inline_transfer(presale_token_sym.get_contract(), _self, FEE_CONTRACT, platform_fee, "Presale platform fee");
}

// 领取预售资金并添加流动性
void launchpad::withdraw(uint64_t pid, name owner) {
    require_auth(owner);

    auto itr = _presales.require_find(pid, "Presale not exists!");
    auto now_time = current_time_point().sec_since_epoch();
    check(now_time >= itr->presale_start_time, "Presale not start!");
    //check(now_time >= itr->presale_end_time, "Presale not end!");
    check(owner == itr->owner, "Invalid owner account!");
    check(itr->presale_get_balance >= itr->soft_cap, "Presale fail!");
    check(!itr->claimed, "Presale amount has already claimed!");
    
    name TOKEN_CONTRACT = itr->presale_token_sym.get_contract();
    asset deposit_eos = itr->presale_get_balance * itr->swap_liquidity / 100;
    asset deposit_token = itr->swap_listing_tokens;
    asset risk_eos = itr->presale_get_balance * 2 / 100;
    asset transfer_risk_eos = itr->presale_get_balance * 195 / 10000;
    check(itr->presale_get_balance > PROTOCOL_FEE, "Presale amount too small!");
    
    asset withdraw_eos = itr->presale_get_balance - deposit_eos - risk_eos - PROTOCOL_FEE;
    uint64_t pair_id = itr->pair_id;
    if(itr->swap_type == 1) pair_id = get_pair_id(itr->presale_token_sym, EOS_EXSYM);
    _presales.modify(itr, same_payer, [&]( auto& a) {
        a.pair_id = pair_id;
        a.claimed_presale_get = withdraw_eos;
        a.claimed = true;
    });

    utils::inline_transfer(EOS_CONTRACT, _self, FEE_CONTRACT, (transfer_risk_eos + PROTOCOL_FEE), "Fee amount");

    // claim presale get amount
    utils::inline_transfer(EOS_CONTRACT, _self, owner, withdraw_eos, "Claim presale amount");

    // deposit swap for liq
    name swap_contract = get_swap_contract(itr->swap_type);
    //check(false, TOKEN_CONTRACT.to_string() + "-" + swap_contract.to_string() + "-" + deposit_token.to_string()+"-"+to_string(pair_id));
    utils::inline_transfer(EOS_CONTRACT, _self, swap_contract, deposit_eos, "deposit,"+to_string(pair_id));
    utils::inline_transfer(TOKEN_CONTRACT, _self, swap_contract, deposit_token, "deposit,"+to_string(pair_id));
    action{permission_level{get_self(), "active"_n}, swap_contract, "deposit"_n, make_tuple(get_self(), pair_id)}.send();
    // deposit after
    action{permission_level{get_self(), "active"_n}, _self, "depositafter"_n, make_tuple(pid)}.send();
}

// 添加流动性后的锁仓操作
void launchpad::depositafter(uint64_t pid) {
    require_auth(get_self());
    
    auto itr = _presales.require_find(pid, "Presale not exists!");
    extended_symbol lp_ext_sym = get_lptoken_from_pairid(itr->swap_type, itr->pair_id);
    extended_asset balance = utils::get_balance( lp_ext_sym, get_self() );
     _presales.modify(itr, same_payer, [&]( auto& a) {
        a.locked_lptoken_balance = balance;
    });

    auto lptoken_contract = get_lptoken_contract(itr->swap_type);
    utils::inline_transfer(lptoken_contract, _self, LOCKED_CONTRACT, balance.quantity, "deposit_launch,"+to_string(itr->liquidity_unlock_time)+","+itr->owner.to_string()+","+to_string(itr->id));
}

// 用户退款或取回资产
void launchpad::claim(uint64_t pid, name owner) {
    require_auth(owner);

    auto itr = _presales.require_find(pid, "Presale not exists!");
    auto now_time = current_time_point().sec_since_epoch();
    check(now_time >= itr->presale_start_time, "Presale not start!");

    users _users(_self, pid);
    auto user_itr = _users.require_find(owner.value, "Not fund user");
    if(now_time < itr->presale_end_time) {
        // 预售没结束，退80%
        asset rerund = user_itr->paid*80/100;
        asset fee = user_itr->paid - rerund;
        utils::inline_transfer(EOS_CONTRACT, _self, owner, rerund, "Refund presale token!");
        utils::inline_transfer(EOS_CONTRACT, _self, POOL_MANAGER, fee, "Refund presale fee token!");
        _presales.modify(itr, same_payer, [&]( auto& a) {
            a.presale_get_balance -= user_itr->paid;
        });
    } else {
        // 预售结束且失败则退100%
        if(itr->presale_get_balance < itr->soft_cap) {
            utils::inline_transfer(EOS_CONTRACT, _self, owner, user_itr->paid, "Refund presale token!");
            _presales.modify(itr, same_payer, [&]( auto& a) {
                a.presale_get_balance -= user_itr->paid;
            });
        } else {
            // 预售成功则取回token
            utils::inline_transfer(itr->presale_token_sym.get_contract(), _self, owner, user_itr->tokens, "Withdraw presale token!");
        }
    }

    _users.erase(user_itr); 
}

void launchpad::on_safetransfer( const name from, const name to, const asset quantity, const string memo) {
    name code = get_first_receiver();
    require_auth( code );
    on_transfer_do(from, to, quantity, memo);
}

void launchpad::on_transfer( const name from, const name to, const asset quantity, const string memo) {
    require_auth( from );
    on_transfer_do(from, to, quantity, memo);
}

void launchpad::on_transfer_do(name from, name to, asset quantity, string memo) {
    if ( to != get_self() || from == "eosio.ram"_n || from == "mine4.defi"_n) return;
    
    name code = get_first_receiver();
    const vector<string> parts = utils::split(memo, ",");
    if(parts.size() == 0) return;
    if(parts[0] == "deposit") {
        auto ext_in = extended_asset{ quantity, code };
        do_deposit(from, ext_in);
    }

    if(parts[0] == "ido") {
        check(EOS_CONTRACT == code && EOS_SYMBOL == quantity.symbol, "Invalid token!");
        uint64_t id = stoll(parts[1].c_str());
        ido(from, quantity, id);
    }
}

void launchpad::do_deposit(name owner, extended_asset value) {
    auto ext_sym = value.get_extended_symbol();
    balances _balances = balances(get_self(), owner.value);
    checksum256 asset_id_hash = utils::hash_asset_id(ext_sym);
    auto balances_by_hash = _balances.get_index<name("assetidhash")>();
    auto balance_itr = balances_by_hash.find(asset_id_hash);
    if(balance_itr == balances_by_hash.end()) {
        auto id = _balances.available_primary_key();
        if (id == 0) id = 1;
        _balances.emplace(_self, [&](auto &a) {
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

void launchpad::ido(name owner, asset quantity, uint64_t presale_id) {
    auto itr = _presales.require_find(presale_id, "Presale not exists!");
    auto whitepair_itr = _whitepairs.require_find(itr->whitepair_id, "whitepair_id not exitst");
    auto now_time = current_time_point().sec_since_epoch();
    liquiditys liqtable(swap::code, whitepair_itr->pair_id);
    auto liq_itr = liqtable.find(owner.value);
    check(liq_itr != liqtable.end(), "The white list qualification must provide a market making limit of at least "+whitepair_itr->min_asset.to_string()+" funds and be locked for " +to_string(whitepair_itr->lock_days)+ " days");

    asset user_eos = swap::get_user_eos(whitepair_itr->pair_id, liq_itr->token.amount);
    check(user_eos >= whitepair_itr->min_asset, "The white list qualification must provide a market making limit of at least "+whitepair_itr->min_asset.to_string()+" funds and be locked for 14 days");

    check(liq_itr->unlock_time >= (itr->presale_start_time + whitepair_itr->lock_days * DAY_PER_SECOND), "The white list qualification must provide a market making limit of at least "+whitepair_itr->min_asset.to_string()+" funds and be locked for "+to_string(whitepair_itr->lock_days)+" days");
    symbol sym = itr->presale_token_sym.get_symbol();
    check(now_time >= itr->presale_start_time, "Presale not start!");
    check(now_time <= itr->presale_end_time, "Presale finish!");
    check(itr->presale_get_balance + quantity <= itr->hard_cap, "Presale finish!");
    _presales.modify(itr, same_payer, [&]( auto& a) {
        a.presale_get_balance += quantity;
    });

    asset tokens = asset(itr->presale_rate.amount * quantity.amount/10000, sym);
    users _users(_self, itr->id);
    auto user_itr = _users.find(owner.value);
    if (user_itr == _users.end()) {
        check(quantity >= itr->min_contribution, "Unreaches minimum contribution!");
        check(quantity <= itr->max_contribution, "Over maximum contribution!");
        _users.emplace(get_self(), [&]( auto& a) {
            a.owner = owner;
            a.paid = quantity;
            a.tokens = tokens;
            a.claimed = false;
        });
    } else {
        check(user_itr->paid + quantity <= itr->max_contribution, "Over maximum contribution!");
        _users.modify(user_itr, same_payer, [&]( auto& a) {
            a.paid += quantity;
            a.tokens += tokens;
            a.claimed = false;
        });
    }

    auto data = make_tuple(owner, quantity, tokens);
    action(permission_level{get_self(), name("active")}, get_self(), name("logpresale"), data).send();
}

void launchpad::setwhitepair(uint64_t pair_id, asset min_asset, uint64_t lock_days) {
    require_auth( POOL_MANAGER );

    check(min_asset.symbol == EOS_SYMBOL, "Invalid EOS Symbol");
    auto id = _whitepairs.available_primary_key();
    if (id == 0) id = 1;
    _whitepairs.emplace(get_self(), [&](auto &a) {
        a.id = id;
        a.pair_id = pair_id;
        a.min_asset = min_asset;
        a.lock_days = lock_days;
    });
}

void launchpad::rmwhitepair(uint64_t id) {
    require_auth( POOL_MANAGER );
    auto itr = _whitepairs.find(id);
    if(itr != _whitepairs.end()) {
        _whitepairs.erase(itr);
    }
}

void launchpad::clear(uint64_t id) {
    require_auth( POOL_MANAGER );

    auto itr = _presales.require_find(id, "Presale not exists!");
    _presales.erase(itr);

    users _users(_self, id);
    auto user_itr = _users.begin();
    while(user_itr != _users.end()) {
        _users.erase(user_itr);
        user_itr = _users.begin();
    }
}

void launchpad::logpresale(name owner, asset pay_amount, asset get_amount) {
    require_auth(get_self());
}