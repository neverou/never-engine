#include "util.h"

#include "logger.h"

#include "std.h"

#include <stdarg.h>

// formatting
#include <stdio.h>
#include <string.h>

// TODO(voidless): do custom formatting instead, because vsnprintf doesnt work with StringViews
String TPrint(StringView fmt, ...) {
    va_list va;
    va_list va2;
    
    va_start(va, fmt);
    va_copy(va2, va);
    
    Size len = vsnprintf(NULL, 0, fmt.data, va);
    
    String str = TempString(len);
    vsnprintf(str.data, str.length + 1, fmt.data, va2);
    
    va_end(va);
    va_end(va2);
    
    return str;
}




#include "allocators.h"

void* TempAlloc(Size size) {
	return PushAlloc(size, &GetAllocators()->frameArena);
}



bool Compare(void* a, void* b, u64 size)
{
    u8* _a = (u8*)a;
    u8* _b = (u8*)b;
    for (u64 i = 0; i < size; i++)
        if (_a[i] != _b[i]) return false;
    return true;
}

void Copy(void* dst, void* src, u64 size)
{
    u8* to   = (u8*)dst;
    u8* from = (u8*)src;
    for (u64 i = 0; i < size; i++)
        to[i] = from[i];
}


void Set(void* at, u64 size, u64 value)
{
    for (u8* it = (u8*)at; it != (u8*)at + size; it++)
        *it = value;
}


// file handler

bool OpenFileHandler(StringView path, TextFileHandler* handler) {
	return Open(&handler->file, path, FILE_READ);
}

void CloseFileHandler(TextFileHandler* handler) {
	Close(&handler->file);
}


char ConsumeChar(TextFileHandler* handler, bool* found) {
	File* file = &handler->file;
	Assert(file != NULL);
    
	char ch = 0;
	if (found)
		*found = Read(file, &ch, sizeof(ch)) > 0;
    
	return ch;
}

String ConsumeNextLine(TextFileHandler* handler, bool* found) {
	String line = TempString(0);
    
	bool fnd = false;
	char ch = 0;
	while ((ch = ConsumeChar(handler, &fnd)) != '\n' && fnd) {
		if (ch == '\r') continue;
		line.Concat(CharStr(&ch));
	}
    
	if (found)
		*found = fnd | (!fnd && line.length > 0); // ~Hack hacky?
    
	return line;
}

//


char ConsumeChar(TextHandler* handler, bool* found) {
	if (handler->head >= handler->source.length) { if(found) *found = false; return 0; }
    
    char ch = handler->source.data[handler->head];
    handler->head++;
    if(found) *found = true;
    return ch;
}

String ConsumeNextLine(TextHandler* handler, bool* found) {
	String line = TempString(0);
    
	bool fnd = false;
	char ch = 0;
	while ((ch = ConsumeChar(handler, &fnd)) != '\n' && fnd) {
		if (ch == '\r') continue;
		line.Concat(CharStr(&ch));
	}
    
	if (found)
		*found = fnd | (!fnd && line.length > 0); // ~Hack hacky?
    
	return line;
}


bool ReadEntireFile(StringView path, Array<u8>* data)
{
    File file;
    if (!Open(&file, path, FILE_READ)) return false;
	defer(Close(&file));
    
    u64 fileSize = FileLength(&file);
    auto rawData = MakeArray<u8>(fileSize, Frame_Arena);
    Read(&file, rawData.data, rawData.size);
    *data = rawData;
    
    return true;
}


// string
s32 FindStringFromLeft(StringView str, char ch) {
	for (u32 i = 0; i < str.length; i++) {
		if (str.data[i] == ch) 
			return i;
	}
	return -1;
}

s32 FindStringFromRight(StringView str, char ch) {
	for (s32 i = str.length - 1; i >= 0; i--) {
		if (str.data[i] == ch) 
			return i;
	}
	return -1;	
}

String Substr(StringView str, u32 start, u32 end) {
	Assert(start <= end);
    
	// ~Refactor clamp the ranges of start and end
    
	String subbed = TempString(end - start);
	for (u32 i = start; i < end; i++) {
		subbed.data[i - start] = str.data[i];
	}
	return subbed;
}


String TrimSpacesLeft(StringView str) {
	for (Size i = 0; i < str.length; i++) {
		if (str.data[i] != ' ' && str.data[i] != '\t') {
			return Substr(str, i, str.length);
		}
	}
    
	String defaultResult = TCopyString(str);
	return defaultResult;
}


String TrimSpacesRight(StringView str) {
	for (Size i = str.length - 1; i >= 0; i--) {
		if (str.data[i] != ' ' && str.data[i] != '\t') {
			return Substr(str, 0, i + 1);
		}
	}
    
	String defaultResult = TCopyString(str);
	return defaultResult;
}

String EatSpaces(StringView str)
{
	return TrimSpacesLeft(TrimSpacesRight(str));
}


DynArray<String> BreakString(StringView str, char breakChar) {
	DynArray<String> parts = MakeDynArray<String>(0, Frame_Arena);
    
	u32 offset = 0;
	for (u32 j = 0; j < str.length; j++) {
		if (str.data[j] == breakChar) {
			if (j > offset)
				ArrayAdd(&parts, Substr(str, offset, j));
			offset = j + 1;
		}
	}
    
	if (offset < str.length) {
		ArrayAdd(&parts, Substr(str, offset, str.length));
	}
    
	return parts;
}

bool Equals(StringView a, StringView b)
{
	if (a.length != b.length) return false;

	for (u32 i = 0; i < a.length; i++)
		if (a.data[i] != b.data[i])
			return false;

	return true;
}

bool IsSpace(char ch)
{
	return (
		ch == ' '  ||
		ch == '\r' ||
		ch == '\n'
	);
}

bool IsDigit(char ch)
{
	return (ch >= '0') && (ch <= '9');
}

bool IsAlpha(char ch)
{
	return (
		(ch >= 'a' && ch <= 'z') ||
		(ch >= 'A' && ch <= 'Z') ||
		(ch == '_')
		);
}


// path
StringView GetPathDirectory(StringView path)
{
	int rightmostSlash = path.length;
	// Except for the last character, otherwise we will get stuck on that folder
	for (int x = 0; x < path.length - 1; x++)
	{
		if (path.data[x] == '/')
			rightmostSlash = x;
	}

	return Substr(path, 0, rightmostSlash);
}

StringView AppendPaths(StringView path0, StringView path1)
{
	String s = TempString();
	s.Concat(path0);
	if (path0.data[path0.length - 1] != '/')
		s.Concat("/");
	s.Concat(path1);
	return s;
}


// string parsing
s32 ParseS32(StringView string, bool* success)
{
	s32 number = 0;
	bool negative = false;

	u64 place = 0;
	bool isNumber = true;
	for (u64 i = 0; i < string.length; i++)
	{
		if (i == 0 && string[i] == '-') { negative = true; continue; }

		isNumber &= IsDigit(string[i]);
		place++;
	}

	if (isNumber)
	{
		// skip the first character if negative (the negative sign)
		for (u64 i = (negative ? 1 : 0); i < string.length; i++)
		{
			char digitChar = string[i];
			Assert(IsDigit(digitChar));
			int digit = digitChar - '0';

			place--;
			number += PowU64(10, place) * digit;
		}
	}

	if (negative) number = -number;

	if (success) *success = isNumber;
	return number;
}

u64 ParseU64(StringView string, bool* success)
{
	u64 number = 0;
	
	u64 place = 0;
	bool isNumber = true;
	for (u64 i = 0; i < string.length; i++)
	{
		isNumber &= IsDigit(string[i]);
		place++;
	}

	if (isNumber)
	{
		for (u64 i = 0; isNumber && i < string.length; i++)
		{
			char digitChar = string[i];
			Assert(IsDigit(digitChar));
			int digit = digitChar - '0';

			place--;
			number += PowU64(10, place) * digit;
		}
	}

	if (success) *success = isNumber;
	return number;
}





float ParseFloat(StringView string, bool* success)
{
	float number = 0;

	bool isNumber  = true;
	bool hasPeriod = false;
	bool negative = false;

	int place = 0;

	for (u64 i = 0; isNumber && i < string.length; i++)
	{
		if (i == 0 && string[i] == '-') { negative = true; continue; }

		bool isPeriod = (string[i] == '.');

		// If isPeriod is true but we already have a period then this is an invalid number!
		isNumber &= !(isPeriod && hasPeriod); 
		hasPeriod |= isPeriod;
		isNumber &= IsDigit(string[i]) || isPeriod;

		if (IsDigit(string[i]) && !hasPeriod) place++;
	}

	if (isNumber)
	{
		for (u64 i = 0; i < string.length; i++)
		{
			char numChar = string[i];
			if (i == 0 && numChar == '-') continue; // its negative

			Assert(IsDigit(numChar) || (numChar == '.'));
			
			if (IsDigit(numChar))
			{
				place--;

				int digit = numChar - '0';
				number += PowF(10, place) * digit;
			}
		}

		if (negative)
			number = -number;
	}

	if (success) *success = isNumber;
	return number;
}
