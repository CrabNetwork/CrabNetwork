#include "../../interfaces/utils.hpp"
#include "../../interfaces/safemath.hpp"

static string ERROR_INVALID_MEMO = "swap: invalid memo (ex: \"swap,<min_return>,<pair_ids>\" or \"deposit,<pair_id>\"";
static string ERROR_CONFIG_NOT_EXISTS = "swap: contract is under maintenance";

struct memo_schema {
   name                 action;
   vector<uint64_t>     pair_ids;
   int64_t              min_return;
};

struct transfer_args {
    name from;
    name to;
    asset quantity;
    std::string memo;
};

static constexpr uint32_t MAX_PROTOCOL_FEE = 100;
static constexpr uint32_t MAX_TRADE_FEE = 50;
static constexpr uint64_t MINIMUM_LIQUIDITY = 1000;
static constexpr uint64_t PRICE_BASE = 10000;

static constexpr name MIN_LP_ACCOUNT = "minlpaccount"_n;
static constexpr name PROTOCOL_FEE_ACCOUNT = "aidaoswapfet"_n;
static constexpr name LPTOKEN_CONTRACT = "swaplptokent"_n;
static constexpr name LPFARM_CONTRACT = "swapswapfarm"_n;
static constexpr name POOL_MANAGER = name("wdogdeployer");

namespace crab {

class [[eosio::contract("swap")]] swap : public eosio::contract {
public:
   using contract::contract;

   ACTION init(const name lptoken_contract);
   ACTION setfee(uint8_t trade_fee, uint8_t protocol_fee, name fee_account);
   ACTION setnotifiers(const vector<name> log_notifiers);
   ACTION setpairnotif(uint64_t pair_id, vector<name> pair_notifiers);
   ACTION createpair(name creator, extended_symbol token0, extended_symbol token1);
   ACTION removepair(uint64_t pair_id);
   ACTION deposit(name owner, uint64_t pair_id);
   ACTION cancel(name owner);
   ACTION lockliq(uint64_t pair_id, name owner, uint32_t day);
   ACTION swaplog( const uint64_t pair_id, const name owner, const name action, const asset quantity_in, const asset quantity_out, const asset fee, const double trade_price, const asset reserve0, const asset reserve1 );
   ACTION liquiditylog( const uint64_t pair_id, const name owner, const name action, const asset liquidity, const asset quantity0, const asset quantity1, const asset total_liquidity, const asset reserve0, const asset reserve1 );
   ACTION tokenchange(symbol_code code, uint64_t pid, name owner, uint64_t pre_amount, uint64_t now_amount);

   using swaplog_action = eosio::action_wrapper<"swaplog"_n, &swap::swaplog>;
   using liquiditylog_action = eosio::action_wrapper<"liquiditylog"_n, &swap::liquiditylog>;
   using tokenchange_action = eosio::action_wrapper<"tokenchange"_n, &swap::tokenchange>;

   [[eosio::on_notify("*::transfer")]]
   void on_transfer(name from, name to, asset quantity, std::string memo);
   [[eosio::on_notify("*::safetransfer")]]
   void on_safetransfer(name from, name to, asset quantity, std::string memo);

private:
   TABLE pair_t {
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
      uint64_t lptoken_code_id() const { return lptoken_code.raw(); };
      checksum256 asset_ids_hash() const { return utils::hash_asset_ids(token0, token1); };
   };

   TABLE liquidity_t {
      name owner;
      asset amount0;
      asset amount1;
      asset token;
      uint64_t unlock_time;

      uint64_t primary_key() const { return owner.value; }
   };

   TABLE config_t {
      uint64_t            pair_id;
      uint8_t             trade_fee = 20;
      uint8_t             protocol_fee = 10;
      name                fee_account = PROTOCOL_FEE_ACCOUNT;
      name                lptoken_contract = LPTOKEN_CONTRACT;
      vector<name>        notifiers = {};
   };

   TABLE lpnotifier_t {
      uint64_t pair_id;
      vector<name> notifiers = {};

      uint64_t primary_key() const { return pair_id; };
   };

   TABLE balances_t {
      uint64_t id;
      extended_symbol sym;
      asset balance;

      uint64_t primary_key() const { return id; };
      checksum256 asset_id_hash() const { return utils::hash_asset_id(sym); };
   };

   TABLE pool_t {
      uint64_t pid;
      extended_symbol want;
      uint64_t weight;
      uint64_t last_reward_time;
      int128_t reward_per_share;
      uint64_t total_rewards;
      int128_t shares_total;  
      asset total_staked;  
      bool display;
      bool enabled;
      
      uint64_t primary_key() const { return pid; }
  };

  TABLE user_t {
      name owner;
      asset staked;
      int128_t shares;
      int128_t reward_debt;
      asset amount0;
      asset amount1;
      uint64_t last_staked_time;
      uint64_t last_withdraw_time;

      uint64_t primary_key() const { return owner.value; }
  }; 

   typedef multi_index <name("balances"), balances_t,
      indexed_by < name("assetidhash"), const_mem_fun < balances_t, checksum256, &balances_t::asset_id_hash>>> balances;
   typedef multi_index<"pairs"_n, pair_t,
      indexed_by < name("lptokencode"), const_mem_fun < pair_t, uint64_t, &pair_t::lptoken_code_id>>,
      indexed_by < name("assetidshash"), const_mem_fun < pair_t, checksum256, &pair_t::asset_ids_hash>>> pairs;
   typedef eosio::singleton<"configs"_n, config_t> configs;
   typedef multi_index<name("configs"), config_t> configs_for_abi;
   typedef multi_index<"liquidity2"_n, liquidity_t> liquiditys;
   typedef multi_index<"pairnotifier"_n, lpnotifier_t> pairnotifiers;
   typedef multi_index<"pools"_n, pool_t> pools;
   typedef multi_index<"users"_n, user_t> users;

   pairs _pairs = pairs(_self, _self.value);
   configs _configs = configs(_self, _self.value);
   pairnotifiers _pairnotifiers = pairnotifiers(_self, _self.value);

private:
   void create( const extended_symbol value );
   void do_swap(const name owner, const extended_asset ext_quantity, const vector<uint64_t> pair_ids, const int64_t min_return );
   void do_deposit(const name owner, const uint64_t pair_id, const extended_asset value);
   void do_withdraw(const name owner, const uint64_t pair_id, const extended_asset value);
   void add_liquidity(name user, uint64_t pair_id);
   std::pair<uint64_t, uint64_t> mint_liquidity_token(uint64_t pair_id, name to, asset quantity, asset amount0, asset amount1);
   std::pair<uint64_t, uint64_t> burn_liquidity_token(uint64_t pair_id, name to, asset quantity, asset amount0, asset amount1);
   void update(uint64_t pair_id, int128_t balance0, int128_t balance1, int128_t reserve0, int128_t reserve1);
   //uint64_t get_mid();
   int128_t quote(int128_t amount0, int128_t reserve0, int128_t reserve1);
   int128_t get_amount_out(int128_t amount_in, int128_t reserve_in, int128_t reserve_out);
   memo_schema parse_memo( const string memo );
   vector<uint64_t> parse_memo_pair_ids( const string memo );
   void on_transfer_do(name from, name to, asset quantity, string memo, name code);
   void lptoken_change(name from, name to, asset quantity, string memo);
   void notifylog();
   void notifylp(uint64_t pair_id);
   double calculate_price( const asset value0, const asset value1 );
   
   uint64_t get_mid() {
      config_t config = _configs.get_or_default(config_t{});
      config.pair_id += 1;
      _configs.set(config, _self);
      return config.pair_id;
   }

   std::pair<uint64_t, extended_symbol> get_create_lptoken() {
      uint64_t id = get_mid();
      bool valid = false;
      while(!valid) {
         valid = utils::valid_pair_id(id);
         if(!valid) id = get_mid();
      }

      auto liquidity_token = utils::get_lptoken_from_pairid(id, LPTOKEN_CONTRACT, "CB");
      return std::make_pair(id, liquidity_token);
   }

   static int64_t mul_amount( const int64_t amount, const uint8_t precision0, const uint8_t precision1 ) {
      const int64_t res = static_cast<int64_t>( precision0 >= precision1 ? safemath::mul(amount, pow(10, precision0 - precision1 )) : amount / static_cast<int64_t>(pow( 10, precision1 - precision0 )));
      check(res >= 0, "mul_amount: mul/div overflow");
      return res;
   }

   static int64_t div_amount( const int64_t amount, const uint8_t precision0, const uint8_t precision1 ) {
      return precision0 >= precision1 ? amount / static_cast<int64_t>(pow( 10, precision0 - precision1 )) : safemath::mul(amount, pow( 10, precision1 - precision0 ));
   }
};
}