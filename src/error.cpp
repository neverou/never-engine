#include "error.h"
#include "logger.h"

void DoAssert(u64 line, StringView file, StringView text) {
    LogError("Assert failed on %lu in %s:%lu (%s)", line, file.data, line, text.data);
}