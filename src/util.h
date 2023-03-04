// NOTE(voidless):
// Instead of using the C-Runtime library,
// we have <util.h> and <sys.h>
// all our versions of the the platform functions in the CRT go in <sys.h>
// and the utility ones go in <util.h> 

#pragma once

#include "core.h"
#include "sys.h"
#include "array.h"
#include "str.h"

#define Offset(st, m) ((u64)(&((st*)0)->m))

// formatting
String TPrint(StringView fmt, ...);

// mem
void* TempAlloc(u64 size);
bool Compare(void* a, void* b, u64 size);
void Copy(void* dst, void* src, u64 size);
void Set(void* at, u64 size, u64 value); 

// file

struct TextFileHandler
{
	File file;
};

bool OpenFileHandler(StringView path, TextFileHandler* handler);
void CloseFileHandler(TextFileHandler* handler);
char ConsumeChar(TextFileHandler* handler, bool* found);
String ConsumeNextLine(TextFileHandler* handler, bool* found);

struct TextHandler
{
	StringView source;
	u64        head;
};

char   ConsumeChar(TextHandler* handler, bool* found);
String ConsumeNextLine(TextHandler* handler, bool* found);


bool ReadEntireFile(StringView path, Array<u8>* data);
// ~Todo recursive list all files


// string
s32 FindStringFromLeft(StringView str, char ch);
s32 FindStringFromRight(StringView str, char ch);
String Substr(StringView str, u32 start, u32 end);

String TrimSpacesLeft(StringView str);
String TrimSpacesRight(StringView str);
String EatSpaces(StringView str);

DynArray<String> BreakString(StringView str, char breakChar);

bool Equals(StringView a, StringView b);
bool IsSpace(char ch);
bool IsDigit(char ch);
bool IsAlpha(char ch);

// path
StringView GetPathDirectory(StringView path);
StringView AppendPaths(StringView path0, StringView path1);


// string parsing
s32 ParseS32(StringView string, bool* success = NULL);
u64 ParseU64(StringView string, bool* success = NULL);

float ParseFloat(StringView string, bool* success = NULL);
