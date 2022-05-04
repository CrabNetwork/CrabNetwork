namespace crab {

// accounts to be notified via inline action
void swap::notifylog()
{
    swap::configs _config( get_self(), get_self().value );
    auto config = _config.get_or_default();

    for ( const name notifier : config.notifiers ) {
        if ( is_account( notifier ) ) require_recipient( notifier );
    }
}

void swap::notifylp(uint64_t pair_id)
{
    swap::pairnotifiers _notifiers(_self, _self.value);
    auto itr = _notifiers.find(pair_id);
    if(itr != _notifiers.end()) {
        for ( const name notifier : itr->notifiers ) {
            if ( is_account( notifier ) ) require_recipient( notifier );
        }
    }

    auto m_itr = _notifiers.find(0);
    if(m_itr != _notifiers.end()) {
        for ( const name notifier : m_itr->notifiers ) {
            if ( is_account( notifier ) ) require_recipient( notifier );
        }
    }
}

[[eosio::action]]
void swap::liquiditylog( const uint64_t pair_id, const name owner, const name action, const asset liquidity, const asset quantity0,  const asset quantity1, const asset total_liquidity, const asset reserve0, const asset reserve1 )
{
    require_auth( get_self() );
    notifylog();
    require_recipient( owner );
}

[[eosio::action]]
void swap::swaplog( const uint64_t pair_id, const name owner, const name action, const asset quantity_in, const asset quantity_out, const asset fee, const double trade_price, const asset reserve0, const asset reserve1 )
{
    require_auth( get_self() );
    notifylog();
    require_recipient( owner );
}

[[eosio::action]]
void swap::tokenchange( symbol_code code, uint64_t pair_id, name owner, uint64_t pre_amount, uint64_t now_amount )
{
    require_auth( get_self() );
    notifylp(pair_id);
    require_recipient( owner );
}


// ACTION swap::tokenchange(symbol_code code, uint64_t pid, name owner, int128_t pre_amount, int128_t now_amount) {
//     require_auth(get_self());
//     require_recipient(LPFARM_CONTRACT);
// }

void swap::create( const extended_symbol value )
{
    asset maximum_supply{1000000000000000, value.get_symbol()};
    action{permission_level{get_self(), "active"_n}, value.get_contract(), "create"_n, std::make_tuple(maximum_supply)}.send(); 
}

// void create(extended_symbol extended_sym) { 
   //    asset maximum_supply{1000000000000000, extended_sym.get_symbol()};
   //    action{permission_level{get_self(), "active"_n}, LPTOKEN_CONTRACT, "create"_n, std::make_tuple(maximum_supply)}.send(); 
   // }

} // namespace sx