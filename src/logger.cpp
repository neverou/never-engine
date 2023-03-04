#include "logger.h"

// TODO(...): replace @CRT 
#include <stdio.h>
#include <stdarg.h>

intern void PrintStringView(StringView msg)
{
	printf("%.*s\n", cast(s32, msg.length), msg.data);
}


intern StringView TPrintVaArgs(StringView fmt, va_list va)
{
    va_list va2;
    
    va_copy(va2, va);
    
    Size len = vsnprintf(NULL, 0, fmt.data, va);
    
    String str = TempString(len);
    vsnprintf(str.data, str.length + 1, fmt.data, va2);
    
    va_end(va2);
    
    return GetStringView(str);
}

void Log(StringView message, ...)
{
    va_list va;
    va_start(va, message);
	PrintStringView(TPrintVaArgs(message, va));
    va_end(va);
}

void LogWarn(StringView message, ...)
{
	String messageBuffer = TempString();
	messageBuffer.Concat("[warn] ");
    
    va_list va;
    va_start(va, message);
	messageBuffer.Concat(TPrintVaArgs(message, va));
    va_end(va);
	
	PrintStringView(GetStringView(messageBuffer));
}

void LogError(StringView message, ...)
{
	String messageBuffer = TempString();
	messageBuffer.Concat("[err] ");
    
    va_list va;
    va_start(va, message);
	messageBuffer.Concat(TPrintVaArgs(message, va));
	va_end(va);
    
	PrintStringView(GetStringView(messageBuffer));
}
