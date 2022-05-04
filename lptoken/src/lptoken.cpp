#include <lptoken.hpp>

void lptoken::create(const asset &maximum_supply) {
   require_auth(SWAP_ACCOUNT);
 
   auto sym = maximum_supply.symbol;
   check(sym.is_valid(), "invalid symbol name");
   check(maximum_supply.is_valid(), "invalid supply");
   check(maximum_supply.amount > 0, "max-supply must be positive");

   stats statstable(get_self(), sym.code().raw());
   auto existing = statstable.find(sym.code().raw());
   check(existing == statstable.end(), "token with symbol already exists");

   statstable.emplace(get_self(), [&](auto &s) {
      s.supply.symbol = maximum_supply.symbol;
      s.max_supply = maximum_supply;
      s.issuer = SWAP_ACCOUNT;
   });

   accounts acnts(get_self(), get_self().value);
   auto it = acnts.find(sym.code().raw());
   if (it == acnts.end()) {
      acnts.emplace(get_self(), [&](auto &a) {
         a.balance = asset{0, sym};
      });
   }
}

void lptoken::modify() {
   require_auth(get_self());
   stats statstable(get_self(), symbol_code{"CRABA"}.raw());
   auto existing = statstable.find(symbol_code{"CRABA"}.raw());
   check(existing != statstable.end(), "token with symbol does not exist, create token before issue");
   const auto &st = *existing;
   statstable.modify(st, same_payer, [&](auto &s) {
      s.issuer = SWAP_ACCOUNT;
   });
}

void lptoken::mint(const name &to, const asset &quantity, const string &memo) {
   auto sym = quantity.symbol;
   check(sym.is_valid(), "invalid symbol name");
   check(memo.size() <= 256, "memo has more than 256 bytes");

   stats statstable(_self, sym.code().raw());
   auto existing = statstable.find(sym.code().raw());
   check(existing != statstable.end(), "token with symbol does not exist, create token before issue");
   const auto &st = *existing;

   require_auth(st.issuer);
   check(quantity.is_valid(), "invalid quantity");
   check(quantity.amount > 0, "must issue positive quantity");
   check(quantity.symbol == st.supply.symbol, "symbol precision mismatch");
   check(quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

   statstable.modify(st, same_payer, [&](auto &s) {
      s.supply += quantity;
   });

   if (to != st.issuer) {
      add_balance(to, quantity, st.issuer);
      //SEND_INLINE_ACTION(*this, transfer, {{st.issuer, "active"_n}}, {st.issuer, to, quantity, memo});
   } else {
      add_balance(st.issuer, quantity, st.issuer);
   }
}

void lptoken::burn(const name &owner, const asset &quantity, const string &memo) {
   auto sym = quantity.symbol;
   check(sym.is_valid(), "invalid symbol name");
   check(memo.size() <= 256, "memo has more than 256 bytes");

   stats statstable(get_self(), sym.code().raw());
   auto existing = statstable.find(sym.code().raw());
   check(existing != statstable.end(), "token with symbol does not exist");
   const auto &st = *existing;

   require_auth(owner);
   check(quantity.is_valid(), "invalid quantity");
   check(quantity.amount > 0, "must retire positive quantity");

   check(quantity.symbol == st.supply.symbol, "symbol precision mismatch");

   statstable.modify(st, same_payer, [&](auto &s) {
      s.supply -= quantity;
   });

   sub_balance(owner, quantity);
}

void lptoken::transfer(const name &from, const name &to, const asset &quantity, const string &memo) {
   check(from != to, "cannot transfer to self");
   require_auth(from);
   check(is_account(to), "to account does not exist");
   auto sym = quantity.symbol.code();
   stats statstable(get_self(), sym.raw());
   const auto &st = statstable.get(sym.raw());

   locks _locks = locks(_self, sym.raw());
   auto itr = _locks.find(from.value);
   if(itr != _locks.end()) {
      auto now_time = current_time_point().sec_since_epoch();
      check(now_time > itr->unlock_time, "Token has locked");
   }

   require_recipient(from);
   require_recipient(to);
   if(from != SWAP_ACCOUNT && to != SWAP_ACCOUNT) {
      require_recipient(SWAP_ACCOUNT);
   }

   check(quantity.is_valid(), "invalid quantity");
   check(quantity.amount > 0, "must transfer positive quantity");
   check(quantity.symbol == st.supply.symbol, "symbol precision mismatch");
   check(memo.size() <= 256, "memo has more than 256 bytes");

   auto payer = has_auth(to) ? to : from;

   sub_balance(from, quantity);
   add_balance(to, quantity, payer);
}

void lptoken::sub_balance(const name &owner, const asset &value) {
   accounts from_acnts(get_self(), owner.value);
   const auto &from = from_acnts.get(value.symbol.code().raw(), "no balance object found");

   auto balance = from.balance.amount;
   check(balance >= value.amount, "overdrawn balance");
  
   from_acnts.modify(from, owner, [&](auto &a) {
      a.balance -= value;
   });
}

void lptoken::add_balance(const name &owner, const asset &value, const name &ram_payer) {
   accounts to_acnts(get_self(), owner.value);
   auto to = to_acnts.find(value.symbol.code().raw());
   if (to == to_acnts.end()) {
      to_acnts.emplace(ram_payer, [&](auto &a) {
         a.balance = value;
      });
   } else {
      to_acnts.modify(to, same_payer, [&](auto &a) {
         a.balance += value;
      });
   }
}

void lptoken::close(const name &owner, const symbol &symbol) {
   require_auth(owner);
   accounts acnts(get_self(), owner.value);
   auto it = acnts.find(symbol.code().raw());
   check(it != acnts.end(), "Balance row already deleted or never existed. Action won't have any effect.");
   check(it->balance.amount == 0, "Cannot close because the balance is not zero.");
   acnts.erase(it);
}

void lptoken::lock(name owner, symbol_code sym_code, uint64_t unlock_time) {
   require_auth(SWAP_ACCOUNT);
   locks _locks = locks(_self, sym_code.raw());
   auto itr = _locks.find(owner.value);
   if (itr == _locks.end()) {
      _locks.emplace(_self, [&](auto &a) {
         a.owner = owner;
         a.unlock_time = unlock_time;
      });
   } else {
      _locks.modify(itr, same_payer, [&](auto &a) {
         a.unlock_time = unlock_time;
      });
   }
}

void lptoken::unlock(name owner, symbol_code sym_code) {
   require_auth(SWAP_ACCOUNT);
   locks _locks = locks(_self, sym_code.raw());
   auto itr = _locks.find(owner.value);
   if (itr != _locks.end()) {
      _locks.erase(itr);
   }
}
