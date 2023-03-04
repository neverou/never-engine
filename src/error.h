#pragma once
#include "str.h"

#include <stdlib.h> // for exit

#if defined(BUILD_DEBUG)
#if defined(PLATFORM_WINDOWS)
#define DbgBreak() { __debugbreak(); }
#elif defined(PLATFORM_LINUX) || defined(PLATFORM_MACOS)
#include <signal.h>
#define DbgBreak() { raise(SIGTRAP); } 
#endif
#else
#define DbgBreak()
#endif

void DoAssert(u64 line, StringView file, StringView text);
#define Assert(x) { if (!(x)) { DoAssert(__LINE__, __FILE__, #x); DbgBreak(); exit(-1); } }
#define AssertMsg(x, ...) { if (!(x)) { LogError(__VA_ARGS__); DoAssert(__LINE__, __FILE__, #x); DbgBreak(); exit(-1); } }

// ~Refactor rename to Error()
#define FatalError(...) { LogError(__VA_ARGS__); DbgBreak(); exit(-1); }