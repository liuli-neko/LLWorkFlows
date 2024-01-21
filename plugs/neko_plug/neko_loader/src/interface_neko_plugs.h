#pragma once
#include "neko_plugs_global.h"

#include <stdint.h>

extern "C" {
struct NekoPlugInstance
{
    void *data;
    int (*count)();
};

struct NekoPlugSymbol {
    const char *functionName;
    const char *features;
};

__export void nekoCreateInstance(struct NekoPlugInstance *instance);
__export void nekoEnumerateInstanceSymbols(struct NekoPlugInstance *instance,struct  NekoPlugSymbol *symbols, uint32_t *symbolCount);
__export void nekoRegistSymbol(struct NekoPlugSymbol *symbol, void *context);
__export bool nekoCheckoutSymbol(struct NekoPlugInstance *instance, struct NekoPlugSymbol *symbol);
__export void nekoReleaseInstance(struct NekoPlugInstance *instance);
__export const char *nekoPlugName();
}
