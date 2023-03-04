#if defined(PLATFORM_LINUX)
#include "sys.h"

#include "std.h"

#include <fcntl.h> // i like how open() is in fcntl.h but close() is in unistd.h
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>

#include <sys/types.h>
#include <dirent.h>

#include "allocators.h"

// filesystem
#include <stdio.h> // TODO(...): replace @CRT


#include <errno.h>

// // ~Todo test this
// intern s32 PosixOpenFlags(u32 mode) {
// 	s32 flags = 0;

// 	if 		((mode & FILE_READ) && (mode & FILE_WRITE)) 	{ flags |= O_RDWR;   }
// 	else if ((mode & FILE_READ) && !(mode & FILE_WRITE))  	{ flags |= O_RDONLY; }
// 	else if ((mode & FILE_WRITE) && !(mode & FILE_READ))  	{ flags |= O_WRONLY; }

// 	if ((mode & FILE_CREATE))   flags |= O_CREAT;
// 	if ((mode & FILE_TRUNCATE)) flags |= O_TRUNC;
// 	if ((mode & FILE_APPEND))   flags |= O_APPEND;

// 	return flags;
// }

// bool Open(File* file, StringView str, u32 mode)
// {
// 	Assert(file);
	
// 	s32 fd = open(str.data, PosixOpenFlags(mode), S_IRUSR | S_IWUSR);

	
// 	// failed to open file
// 	if (fd == -1) {
// 		LogWarn("[sys] Failed to open file \"%s\"", str.data);
// 		return false;
// 	}

// 	#if defined(BUILD_DEBUG)
// 		strcpy(file->_filename, str.data);
// 	#endif

// 	file->error = false;
// 	file->mode = mode;
// 	file->fileDescriptor = fd;

// 	return true;
// }

// void Close(File* file)
// {
// 	Assert(file);
	
// 	if (file->error)
// 	{
// 		#if defined(BUILD_DEBUG)
// 			LogWarn("[sys] File %d (%s) was closed with errors!", file->fileDescriptor, file->_filename);
// 		#else
// 			LogWarn("[sys] File %d was closed with errors!", file->fileDescriptor);
// 		#endif
// 	}

// 	// ~Hack no clue why this would fail, so if we run into issues we can fix them then
// 	Assert(close(file->fileDescriptor) != -1);
// 	file->fileDescriptor = -1;
// }


// Size Read(File* file, void* buffer, Size count) {
// 	Assert(file);
// 	Assert(file->mode & FILE_READ);
	
// 	// ~Hack ssize_t not standard in engine
// 	ssize_t amtRead = read(file->fileDescriptor, buffer, count);
// 	if (amtRead == -1) {
// 		LogError("Failed to read file: %s (errno=%lu)", strerror(errno), errno);
// 		file->error = true;
// 	}

// 	return amtRead;
// }

// Size Write(File* file, void* buffer, Size count) {
// 	Assert(file);
// 	Assert(file->mode & FILE_WRITE);

// 	// ~Hack ssize_t not standard in engine
// 	ssize_t amtWritten = write(file->fileDescriptor, buffer, count);
// 	if (amtWritten == -1)
// 	{
// 		LogError("Failed to write file: %s (errno=%lu)", strerror(errno), errno);
// 		file->error = true;
// 	}

// 	return amtWritten;
// }

// Size FileLength(File* file) {
// 	Assert(file);

// 	struct stat fileStat;
// 	s32 err = fstat(file->fileDescriptor, &fileStat);

// 	if (err == -1)
// 	{
// 		LogError("Failed to get file length: %s (errno=%lu)", strerror(errno), errno);
// 		file->error = true;
// 	}	

// 	return fileStat.st_size; 
// }






bool Open(File* file, StringView str, u32 mode) {
	FILE* f = NULL;

	const char* openMode = "";
	// ~FixMe
	if (mode & FILE_READ)
	{
		if (mode & FILE_WRITE)
		{
			Assert((mode & FILE_APPEND) != (mode & FILE_TRUNCATE));
			if (mode & FILE_APPEND)
			{
				openMode = "a+b";
			}
			else if (mode & FILE_TRUNCATE)
			{
				openMode = "w+b";
			}
		}
		else
		{
			Assert(!(mode & FILE_APPEND));
			Assert(!(mode & FILE_TRUNCATE));
			openMode = "rb";
		}
	}
	else if (mode & FILE_WRITE)
	{
		Assert((mode & FILE_APPEND) != (mode & FILE_TRUNCATE));
		if (mode & FILE_APPEND)
		{
			openMode = "ab";
		}
		else if (mode & FILE_TRUNCATE)
		{
			openMode = "wb";
		}
	}

	f = fopen(str.data, openMode);

	if (!f)
		return false;

	file->fileHandle = f;
	file->mode = mode;
	
	return true;
}

void Close(File* file) {
	fclose((FILE*)file->fileHandle);
}

Size Read(File* file, void* buffer, Size count) {
	return fread(buffer, 1, count, (FILE*)file->fileHandle);
}

Size Write(File* file, void* buffer, Size count) {
	return fwrite(buffer, 1, count, (FILE*)file->fileHandle);
}

Size FileLength(File* file) {
	FILE* f = (FILE*)file->fileHandle;
	Size offset = ftell(f);
	fseek(f, 0, SEEK_END);
	Size len = ftell(f);
	fseek(f, offset, SEEK_SET);
	return len;
}
/// 






Array<String> ListAllDirs(StringView path)
{
	DynArray<String> directories = MakeDynArray<String>(0, Frame_Arena);

	struct dirent* dent;
    DIR* srcdir = opendir(path.data);

    if (srcdir == NULL)
    {
        LogWarn("[sys] opendir error while indexing directory!");
		return directories;
    }

    while((dent = readdir(srcdir)) != NULL)
    {
        struct stat st;

        if(strcmp(dent->d_name, ".") == 0 || strcmp(dent->d_name, "..") == 0)
            continue;

        if (fstatat(dirfd(srcdir), dent->d_name, &st, 0) < 0)
        {
            LogWarn("[sys] something to do with %s", dent->d_name);
            continue;
        }

        if (S_ISDIR(st.st_mode)) 
			ArrayAdd(&directories, TCopyString(dent->d_name));
    }
    closedir(srcdir);

	return directories;
}

Array<String> ListAllFiles(StringView path)
{
	DynArray<String> files = MakeDynArray<String>(0, Frame_Arena);

	struct dirent* dent;
    DIR* srcdir = opendir(path.data);

    if (srcdir == NULL)
    {
        LogWarn("[sys] opendir error while indexing directory!");
		return files;
    }

    while((dent = readdir(srcdir)) != NULL)
    {
        struct stat st;

        if(strcmp(dent->d_name, ".") == 0 || strcmp(dent->d_name, "..") == 0)
            continue;

        if (fstatat(dirfd(srcdir), dent->d_name, &st, 0) < 0)
        {
            LogWarn("[sys] something to do with %s", dent->d_name);
            continue;
        }

        if (!S_ISDIR(st.st_mode)) 
			ArrayAdd(&files, TCopyString(dent->d_name));
    }
    closedir(srcdir);

	return files;
}







void Memset(void* data, Size size, u8 value) {
	memset(data, value, size);
}

void Memcpy(void* dst, void* src, Size size) {
	memcpy(dst, src, size);
}


//

// ~Todo SDL_GetTicks64 or replace with custom platform function
#include <sdl/SDL.h>
u64 GetTicks() { return SDL_GetTicks(); }

#endif
