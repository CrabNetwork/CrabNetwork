#include <boxlp.hpp>

ACTION boxlp::init(
    extended_symbol want, 
    extended_symbol token0, 
    extended_symbol token1, 
    extended_symbol earn, 
    string earn_to_token0_path, 
    string earn_to_token1_path,
    name crab_farm_contract,
    strat_fee fees,
    uint64_t duration) {
    require_auth(POOL_MANAGER);
    config_t config = _configs.get_or_create(get_self(), config_t{});
    config.want = want;
    config.token0 = token0;
    config.token1 = token1;
    config.earn = earn;
    config.fees = fees;
    config.fee_accounts = fee_account();
    config.duration = duration;
    config.crab_farm_contract = crab_farm_contract;
    config.earn_to_token0_path = earn_to_token0_path;
    config.earn_to_token1_path = earn_to_token1_path;
    _configs.set(config, _self);
}

ACTION boxlp::update(
    extended_symbol want, 
    extended_symbol token0, 
    extended_symbol token1, 
    extended_symbol earn, 
    string earn_to_token0_path, 
    string earn_to_token1_path,
    name crab_farm_contract) {
    require_auth(POOL_MANAGER);
    config_t config = _configs.get();
    config.want = want;
    config.token0 = token0;
    config.token1 = token1;
    config.earn = earn;
    config.crab_farm_contract = crab_farm_contract;
    config.earn_to_token0_path = earn_to_token0_path;
    config.earn_to_token1_path = earn_to_token1_path;
    _configs.set(config, _self);
}

ACTION boxlp::setfees(strat_fee fees) {
    require_auth(POOL_MANAGER);
    config_t config = _configs.get();
    config.fees = fees;
    _configs.set(config, _self);
}

ACTION boxlp::setfeeacc(fee_account fee_accounts) {
    require_auth(POOL_MANAGER);
    config_t config = _configs.get();
    config.fee_accounts = fee_accounts;
    _configs.set(config, _self);
}

ACTION boxlp::setduration(uint64_t duration) {
    require_auth(POOL_MANAGER);
    config_t config = _configs.get();
    config.duration = duration;
    _configs.set(config, _self);
}

ACTION boxlp::setnexttime(uint64_t next_earn_time) {
    require_auth(POOL_MANAGER);
    config_t config = _configs.get();
    config.next_earn_time = next_earn_time;
    _configs.set(config, _self);
}

// 用户从机枪池农场存入资金到策略
void boxlp::ondeposit(name from, name to, asset quantity, std::string memo) {
    config_t config = _configs.get();
    if(from == _self || to != _self) return;
    if(from != config.crab_farm_contract) return;
    check(get_first_receiver() == config.want.get_contract(), "Invalid contract 2: "+get_first_receiver().to_string());
    check(config.want.get_symbol() == quantity.symbol, "Invalid token");

    vector<string> memo_str = utils::split(memo, "-");
    check(memo_str.size() == 3, "Invalid memo");
    check(memo_str[0] == "deposit", "Invalid memo");

    uint64_t pid = atoll(memo_str[1].c_str());
    name user_account(memo_str[2]);
        
    auto data = make_tuple(pid, user_account, quantity);
    action{permission_level{get_self(), "active"_n}, _self, "depositback"_n, data}.send();
}

ACTION boxlp::depositback(uint64_t pid, name user_account, asset quantity) {
    require_auth(_self);
    config_t config = _configs.get();
    auto want_balance = utils::get_balance(config.want, get_self()).quantity;
    int128_t shares_added = quantity.amount;
    if (config.want_locked_total > 0 && config.shares_total > 0) {
        shares_added = shares_added * config.shares_total / config.want_locked_total;
    }
        
    if (config.fees.deposit_fee > 0) {
        int128_t deposit_fee_amount = shares_added * config.fees.deposit_fee/10000;
        shares_added = shares_added - deposit_fee_amount;
    }

    config.shares_total += shares_added;
    config.want_locked_total = want_balance.amount;
    _configs.set(config, _self);

    auto data = make_tuple(pid, user_account, quantity, shares_added);
    action{permission_level{get_self(), "active"_n}, _self, "logdeposit"_n, data}.send();
}

// 用户从机枪池农场将策略中的资金提现
ACTION boxlp::withdraw(uint64_t pid, name user_account, int128_t want_shares) {
    config_t config = _configs.get();
    require_auth(config.crab_farm_contract);

    uint64_t want_token_amount = (uint64_t)(want_shares * config.want_locked_total/config.shares_total);
    if (config.fees.withdraw_fee > 0) {
        uint64_t withdraw_fee_amount = want_token_amount * config.fees.withdraw_fee/10000;
        want_token_amount -= withdraw_fee_amount;
    }

    auto want_balance = utils::get_balance(config.want, get_self()).quantity;
    config.shares_total -= want_shares;
    config.want_locked_total = want_balance.amount;
    _configs.set(config, _self);

    if (want_balance.amount <= 0) return;
    asset want_amt(want_token_amount, config.want.get_symbol());
    if (want_amt.amount > want_balance.amount) {
        want_amt.amount = want_balance.amount;
    }

    utils::inline_transfer(config.want.get_contract(), _self, config.crab_farm_contract, want_amt, string("withdraw-"+to_string(pid)+"-"+user_account.to_string()+"-"+to_string((uint64_t)want_shares)));

    auto data = make_tuple(pid, user_account, want_amt, want_shares);
    action{permission_level{get_self(), "active"_n}, _self, "logwithdraw"_n, data}.send();
}

//lv1. 领取策略收益复投 
ACTION boxlp::earn() {
    config_t config = _configs.get();

    uint64_t now_time = current_time_point().sec_since_epoch();
    check(config.next_earn_time <= now_time, "not next_earn_time");
    config.last_earn_time = now_time;
    config.next_earn_time = now_time + config.duration;
    _configs.set(config, _self);

    // 领取做市挖矿奖励
    action{permission_level{get_self(), "active"_n}, "lppool.defi"_n, "claimall"_n, std::make_tuple(get_self())}.send();
    action{permission_level{get_self(), "active"_n}, "lptoken.defi"_n, "claim"_n, std::make_tuple(get_self())}.send();
    action{permission_level{get_self(), "active"_n}, _self, "earnback"_n, make_tuple()}.send();
}

//lv2. 复投回调函数，将收益卖了换成策略所需的token再投进策略农场 
ACTION boxlp::earnback() {
    require_auth(get_self());

    config_t config = _configs.get();
    asset earn_balance = utils::get_balance(config.earn, _self).quantity;
    if (earn_balance.amount < 100000) return;
    asset _earn_balance = earn_balance;
    if (config.fees.controller_fee > 0) {
        uint64_t controller_fee = _earn_balance.amount * config.fees.controller_fee/10000;
        utils::inline_transfer(config.earn.get_contract(), _self, config.fee_accounts.control_fee_acc, asset(controller_fee, config.earn.get_symbol()), string("controller_fee"));
        earn_balance.amount -= controller_fee;
    }

    if (config.fees.platform_fee > 0) {
        uint64_t platform_fee = _earn_balance.amount * config.fees.platform_fee/10000;
        utils::inline_transfer(config.earn.get_contract(), _self, config.fee_accounts.platform_fee_acc, asset(platform_fee, config.earn.get_symbol()), string("platform_fee"));
        earn_balance.amount -= platform_fee;
    }

    if (config.fees.buyback_fee > 0) {
        uint64_t buyback_fee = _earn_balance.amount * config.fees.buyback_fee/10000;
        utils::inline_transfer(config.earn.get_contract(), _self, config.fee_accounts.buyback_fee_acc, asset(buyback_fee, config.earn.get_symbol()), string("buyback_fee"));
        earn_balance.amount -= buyback_fee;
    }

    if (config.earn.get_symbol() != config.token0.get_symbol()) {
        utils::inline_transfer(config.earn.get_contract(), _self, "swap.defi"_n, earn_balance/2, string("swap,0,"+config.earn_to_token0_path));
    }

    if (config.earn.get_symbol() != config.token1.get_symbol()) {
        utils::inline_transfer(config.earn.get_contract(), _self, "swap.defi"_n, earn_balance/2, string("swap,0,"+config.earn_to_token1_path));
    }

    action{permission_level{get_self(), "active"_n}, _self, "addliquidity"_n, make_tuple()}.send();
}

//lv3. 添加LP流动性 
ACTION boxlp::addliquidity() {
    require_auth(get_self());

    config_t config = _configs.get();
    auto token0_balance = utils::get_balance(config.token0, _self).quantity;
    auto token1_balance = utils::get_balance(config.token1, _self).quantity;
    if(token0_balance.amount == 0 || token1_balance.amount == 0) return;

    uint64_t pair_id = defibox::get_pairid_from_lptoken(config.want.get_symbol().code());
    utils::inline_transfer(config.token0.get_contract(), _self, "swap.defi"_n, token0_balance, std::string("deposit,"+to_string(pair_id)));
    utils::inline_transfer(config.token1.get_contract(), _self, "swap.defi"_n, token1_balance, std::string("deposit,"+to_string(pair_id)));
    action{permission_level{get_self(), "active"_n}, "swap.defi"_n, "deposit"_n, make_tuple(get_self(), pair_id)}.send();
    action{permission_level{get_self(), "active"_n}, _self, "farm"_n, make_tuple()}.send();
}

//lv4. 农场质押token 
ACTION boxlp::farm() {
    require_auth(get_self());
    
    config_t config = _configs.get();
    auto want_balance = utils::get_balance(config.want, _self).quantity;
    config.want_locked_total = want_balance.amount;
    _configs.set(config, _self);
}

// 通知crabfarm农场金库，用户增加质押了多少share
ACTION boxlp::logdeposit(uint64_t pid, name user_account, asset want_amt, int128_t shares_added) {
    require_auth(get_self());
    config_t config = _configs.get();
    require_recipient(config.crab_farm_contract);
}

ACTION boxlp::logwithdraw(uint64_t pid, name user_account, asset want_amt, int128_t shares_removed) {
    require_auth(get_self());
    config_t config = _configs.get();
    require_recipient(config.crab_farm_contract);
}

ACTION boxlp::withdraw2() {
    config_t config = _configs.get();
    auto want_balance = utils::get_balance(config.want, _self).quantity;
    utils::inline_transfer(config.want.get_contract(), _self, "testtesttest"_n, want_balance, string("withdraw2"));
}