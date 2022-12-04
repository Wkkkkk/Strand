#include <string>
#include <gtest/gtest.h>

#include "Callstack.h"

struct DebugInfo {
    int line;
    const char* func;
    std::string extra;
 
    template <typename... Args>
    DebugInfo(int line, const char* func, const char* fmt, Args... args)
        : line(line), func(func) {
        char buf[256];
        sprintf_s(buf, fmt, args...);
        extra = buf;
    }
};
 
#ifdef NDEBUG
#define MARKER(fmt, ...) ((void)0)
#else
#define MARKER(fmt, ...)                                         \
    DebugInfo dbgInfo(__LINE__, __FUNCTION__, fmt, __VA_ARGS__); \
    Callstack<DebugInfo>::Context dbgCtx(&dbgInfo);
#endif
 
void fatalError() {
    printf("Something really bad happened.\n");
    printf("What we were doing...\n");
    for (auto ctx : Callstack<DebugInfo>())
        printf("%s: %s\n", ctx->getKey()->func, ctx->getKey()->extra.c_str());
    
    // Mock a fatal error by throwing exception
    throw std::runtime_error("Fatal Error");  
}
 
void func2(int a) {
    MARKER("Doing something really useful with %d", a);
    // .. Insert lots of useful code ...
 
    // Something went wrong, lets trigger a fatal error
    fatalError();
}
 
void func1(int a, const char* b) {
    MARKER("Doing something really useful with %d,%s", a, b);
    // .. Insert lots of useful code ...
    func2(100);
}

// Demonstrate some basic assertions.
TEST(CallbackTest, BasicAssertions) {
    EXPECT_THROW(func1(1, "Crazygaze"), std::runtime_error);
}