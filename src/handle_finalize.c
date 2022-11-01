#include "harvest_plugin.h"

bool eq(const char *p1, const char *p2) {
    const unsigned char *s1 = (const unsigned char *) p1;
    const unsigned char *s2 = (const unsigned char *) p2;
    unsigned char c1, c2;
    do {
        c1 = (unsigned char) *s1++;
        c2 = (unsigned char) *s2++;
        if (c1 == '\0') return (c1 - c2) == 0;
    } while (c1 == c2);

    return (c1 - c2) == 0;
}

// I have strange plugin crash with Signal 11 when tried read from predefined array of
// contract_info_t so used simple if-else-if chain
#define _HARVEST(address, underlying_ticker, underlying_decimals, vault_ticker, vault_decimals) \
    if (eq(a, address))                                                                         \
        return (contract_info_t){underlying_ticker,                                             \
                                 underlying_decimals,                                           \
                                 vault_ticker,                                                  \
                                 vault_decimals};                                               \
    else

contract_info_t find_contract_info(const char *a) {
// this file is autogenerated by script:
// /tests/harvest/update-contracts-info-b2c.mjs
// or type: cd tests && npm run update-harvest
#include "contracts-info.txt"
#include "contracts-info-tests.txt"
    // when contract not found
    return (contract_info_t){"", 0, "", 0};
}

void handle_finalize(void *parameters) {
    ethPluginFinalize_t *msg = (ethPluginFinalize_t *) parameters;
    context_t *context = (context_t *) msg->pluginContext;

    selector_t selectorIndex = context->selectorIndex;
    msg->numScreens = selectorIndex == POOL_GET_REWARD || selectorIndex == POOL_EXIT ? 1 : 2;

    // Fill context underlying and vault ticker/decimals
    char *addr = context->contract_address;
    addr[0] = '0';
    addr[1] = 'x';

    uint64_t chainId = 0;
    getEthAddressStringFromBinary(msg->pluginSharedRO->txContent->destination,
                                  addr + 2,  // +2 here because we've already prefixed with '0x'.
                                  msg->pluginSharedRW->sha3,
                                  chainId);
    PRINTF("MSG Address: %s\n", addr);

    contract_info_t info = find_contract_info(addr);
    if (info.underlying_decimals == 0 &&
        info.vault_decimals == 0) {  // if contract info is not found

        msg->result = ETH_PLUGIN_RESULT_UNAVAILABLE;
    } else {
        PRINTF("info.underlying decimals: %d, ticker: %s\n ",
               info.underlying_decimals,
               info.underlying_ticker);
        PRINTF("info.vault      decimals: %d, ticker: %s\n ",
               info.vault_decimals,
               info.vault_ticker);

        strlcpy(context->underlying_ticker,
                info.underlying_ticker,
                sizeof(context->underlying_ticker));
        context->underlying_decimals = info.underlying_decimals;
        strlcpy(context->vault_ticker, info.vault_ticker, sizeof(context->vault_ticker));
        context->vault_decimals = info.vault_decimals;

        msg->uiType = ETH_UI_TYPE_GENERIC;
        msg->result = ETH_PLUGIN_RESULT_OK;
    }
}
