/* 
DBC = Design By Contract

account.c — C23 usage of dbc_assert.h 

- DBC_MODULE_NAME avoids repeating __FILE__ per assertion — one static string per translation unit.
- Numeric labels (10, 11, 20...) instead of stringified conditions — rationale: no condition text baked into the binary.
- DBC_DISABLE is this library's equivalent of NDEBUG — off by default
- auto for old_balance is C23's type inference (N3007), not C++'s 
     — legal in this GCC/C23 setup, useful exactly for old-value snapshots in postconditions.

gcc -std=c23 account.c -o account                 # contracts active
gcc -std=c23 -DDBC_DISABLE account.c -o account   # contracts compiled out

GNUC 15 or above
*/
#include <stdio.h>
#include <stdlib.h>
#include "dbc_assert.h"

DBC_MODULE_NAME("account")

typedef struct {
    int balance;
    int id;
} Account;

/* Required once per project — the actual violation response. */
DBC_NORETURN void DBC_fault_handler(char const *module, int label) {
    fprintf(stderr, "DBC FAULT: module=%s label=%d\n", module, label);
    abort();   /* swap for fail-safe reset on embedded targets */
}

static void account_invariant(const Account *a) {
    DBC_INVARIANT(1, a->balance >= 0);
    DBC_INVARIANT(2, a->id != 0);
}

[[nodiscard]]
int deposit(Account *a, int amount) {
    DBC_REQUIRE(10, amount > 0);
    account_invariant(a);

    auto old_balance = a->balance;          /* C23 type inference */
    a->balance += amount;

    DBC_ENSURE(11, a->balance == old_balance + amount);
    account_invariant(a);
    return a->balance;
}

[[nodiscard]]
int withdraw(Account *a, int amount) {
    DBC_REQUIRE(20, amount > 0);
    DBC_REQUIRE(21, amount <= a->balance);
    account_invariant(a);

    auto old_balance = a->balance;
    a->balance -= amount;

    DBC_ENSURE(22, a->balance == old_balance - amount);
    account_invariant(a);
    return a->balance;
}

int main(void) {
    Account acc = { .balance = 100, .id = 42 };

    deposit(&acc, 50);
    withdraw(&acc, 30);
    printf("balance = %d\n", acc.balance);

    withdraw(&acc, 1000);   /* violates DBC_REQUIRE(21,...) -> DBC_fault_handler fires */
    return 0;
}