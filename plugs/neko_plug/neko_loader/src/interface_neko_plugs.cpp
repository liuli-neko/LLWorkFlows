#include "interface_neko_plugs.h"
#include "log.hpp"

int kInstanceCount = 0;
int instanceCount() {
    return kInstanceCount;
}

#ifdef _WIN32
#include "Windows.h"
#undef min
#undef max

void nekoCreateInstance(struct NekoPlugInstance *instance) {
    instance->data = nullptr;
    instance->count = instanceCount;
    LOG_INFO("create instance");
}
void nekoEnumerateInstanceSymbols(struct NekoPlugInstance *instance,struct  NekoPlugSymbol *symbols, uint32_t *symbolCount) {
    
}
void nekoRegistSymbol(struct NekoPlugSymbol *symbol, void *context) {
    
}
bool nekoCheckoutSymbol(struct NekoPlugInstance *instance, struct NekoPlugSymbol *symbol) {

}
void nekoReleaseInstance(struct NekoPlugInstance *instance) {

}

char *nekoPlugName() 
{ 
    return "neko-plug";
}

#elif defined(__linux__)
#include <dlfcn.h>

void nekoCreateInstance(struct NekoPlugInstance *instance) {
    instance->data = nullptr;
    instance->count = instanceCount;
    kInstanceCount ++;
    LOG_INFO("%s : create instance(%p)", nekoPlugName(), instance->data);
}
void nekoEnumerateInstanceSymbols(struct NekoPlugInstance *instance,struct  NekoPlugSymbol *symbols, uint32_t *symbolCount) {
    if (instance->data) {

    }
}
void nekoRegistSymbol(struct NekoPlugSymbol *symbol, void *context);
bool nekoCheckoutSymbol(struct NekoPlugInstance *instance, struct NekoPlugSymbol *symbol);
void nekoReleaseInstance(struct NekoPlugInstance *instance) {
    if (instance->data != nullptr) {
        delete instance->data;
        instance->data = nullptr;
        kInstanceCount --;
    }
}
#endif