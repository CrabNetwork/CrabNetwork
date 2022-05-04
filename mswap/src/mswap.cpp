#include <mswap.hpp>

void mswap::on_transfer( const name from, const name to, const asset quantity, const string memo ) {
    require_auth( from );
    if ( to != get_self() || from == "eosio.ram"_n || from == "mine4.defi"_n) return; 

    name code = get_first_receiver();
    if(from == "mine2.defi"_n) utils::inline_transfer(code, _self, "aidaoswapfee"_n, quantity, string("mine box"));
    
    do_swap(from, to, quantity, memo, code);
}

void mswap::do_swap(const name from, const name to, const asset quantity, const string memo, name code) {
    vector<string> mids = utils::split(memo, ",");
    if(mids[0] != "mswap") return;

    std::vector<std::pair<string, uint64_t>> swap_pairs;
    for(int i = 1; i < mids.size(); i++) {
        if(mids[i] != "nil") {
            auto pair = std::pair<string, uint64_t>{ get_swap(i), atoll(mids[i].c_str()) };
            swap_pairs.push_back(pair);
        }
    }

    if (swap_pairs.size() == 0) return;

    extended_symbol out_sym;
    int128_t total_reserve_in = 0;
    for(int i = 0; i < swap_pairs.size(); i++) {
        const auto [ swap_name, mid ] = swap_pairs[i];
        const auto [ reserve_in, reserve_out ] = get_reserves(swap_name, mid, quantity.symbol);
        total_reserve_in += reserve_in.amount;
        out_sym = get_out_extended_sym(swap_name, mid, quantity.symbol);
    }

    uint64_t transfered_amount = 0;
    for(int i = 0; i < swap_pairs.size(); i++) {
        const auto [ swap_name, mid ] = swap_pairs[i];
        const auto [ reserve_in, reserve_out ] = get_reserves(swap_name, mid, quantity.symbol);
        double p = static_cast<double>(reserve_in.amount)/total_reserve_in;
        uint64_t amount = static_cast<uint64_t>(p * quantity.amount);
        uint64_t e_amount = get_amount_out(amount, reserve_in.amount, reserve_out.amount);
        if(e_amount == 0) continue;

        name contract = get_swap_contract(swap_name);
        string swap_memo = get_swap_memo(swap_name, mid);
        if(swap_pairs.size() == 1) {
            if(quantity.amount > 0) utils::inline_transfer(code, _self, contract, quantity, swap_memo);
            break; 
        }

        if (i == swap_pairs.size() - 1) {
            amount = quantity.amount - transfered_amount;
            if(amount > 0) utils::inline_transfer(code, _self, contract, asset(amount, quantity.symbol), swap_memo);
            break;
        }

        if(amount > 0) utils::inline_transfer(code, _self, contract, asset(amount, quantity.symbol), swap_memo);
        transfered_amount += amount;
    }

    auto data = make_tuple(from, quantity, out_sym);
    action(permission_level{_self, "active"_n}, _self, "afterswap"_n, data).send();
}

int128_t mswap::get_amount_out(int128_t amount_in, int128_t reserve_in, int128_t reserve_out) {
    if(amount_in <= 0) return 0;
    if(reserve_in <= 0 || reserve_out <= 0) return 0;
    
    int128_t amount_in_with_fee = amount_in * (PRICE_BASE - 30);
    int128_t numerator = amount_in_with_fee * reserve_out;
    int128_t denominator = reserve_in * PRICE_BASE + amount_in_with_fee;
    int128_t amount_out = numerator / denominator;
    if(amount_out <= 0) return 0;
    return amount_out;
}

void mswap::afterswap(const name owner, const asset in_quantity, extended_symbol out_sym) {
    require_auth( get_self() );
    auto balance = utils::get_balance(out_sym, get_self());
    if(balance.quantity.amount > 0) utils::inline_transfer(out_sym.get_contract(), _self, owner, balance.quantity, string("swap success"));
}