#include "fileMgr.h"

#include <assert.h>
#ifdef WIN32
#include <windows.h>
#else
#include <stdio.h>
#endif
#include <stdlib.h>
struct file_t
{
#ifdef WIN32
    HANDLE hFile;
#else
    FILE *hFile;
#endif
    size_t write(const char *data, size_t sz) {
#ifdef WIN32
        DWORD BT;
        WriteFile(file->hFile, (LPCVOID)data, sz, &BT, NULL);
        return sizeof(T); // FIXME: this should return 0 in case of an error ?
#else
        return fwrite(data,sz,1,hFile);
#endif

    }
    size_t read(char *tgt,size_t sz) {
#ifdef WIN32
        DWORD BT;
        ReadFile(file->hFile, (LPVOID)tgt, sz, &BT, NULL);
#else
        return fread(tgt,sz,1,hFile);
#endif
    }
    template<class T>
    size_t write(const T data) {
        return write((const char *)&data,sizeof(T));
    }

    template<class T>
    size_t read(T &tgt) {
        return read((char *)&tgt,sizeof(T));
    }
    uint32_t size() {
#ifdef WIN32
        return GetFileSize(hFile, NULL);
#else
        long pos = ftell(hFile);
        fseek(hFile,0,SEEK_END);
        long fsz = ftell(hFile);
        fseek(hFile,pos,SEEK_SET);
        return fsz;
#endif

    }
};

file_t *fileMgr_open(const char *name)
{
#ifdef WIN32
	HANDLE hFile = CreateFile((LPCSTR)name, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if( hFile == INVALID_HANDLE_VALUE )
		return NULL;
#else
    FILE * hFile = fopen(name,"rb");
    if(!hFile)
        return NULL;
#endif
	file_t *file = (file_t*)malloc(sizeof(file_t));
	file->hFile = hFile;
	return file;
}

file_t *fileMgr_create(const char * name)
{
#ifdef WIN32
    HANDLE hFile = CreateFile((LPCSTR)name, FILE_ALL_ACCESS, FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
	if( hFile == INVALID_HANDLE_VALUE )
		return NULL;
#else
    FILE *hFile = fopen(name,"w+");
#endif
	file_t *file = (file_t*)malloc(sizeof(file_t));
	file->hFile = hFile;
	return file;
}

void fileMgr_close(file_t *file)
{
	if( file )
	{
#ifdef WIN32
        CloseHandle(file->hFile);
#else
        fclose(file->hFile);
#endif
		free((void*)file);
	}
}


// todo: add cache for writing
void fileMgr_writeS8(file_t *file, int8_t value)
{
    file->write(value);
}

void fileMgr_writeS16(file_t *file, int16_t value)
{
    file->write(value);
}

void fileMgr_writeS32(file_t *file, int32_t value)
{
    file->write(value);
}

void fileMgr_writeU8(file_t *file, uint8_t value)
{
    file->write(value);
}

void fileMgr_writeU16(file_t *file, uint16_t value)
{
    file->write(value);
}

void fileMgr_writeU32(file_t *file, uint32_t value)
{
    file->write(value);
}

void fileMgr_writeFloat(file_t *file, float value)
{
    file->write(value);
}

void fileMgr_writeData(file_t *file, void *data, int32_t len)
{
    file->write((const char *)data,len);
}


int8_t fileMgr_readS8(file_t *file)
{
    int8_t value;
    file->read(value);
	return value;
}

int16_t fileMgr_readS16(file_t *file)
{
    int16_t value;
    file->read(value);
    return value;
}

int32_t fileMgr_readS32(file_t *file)
{
    int32_t value;
    file->read(value);
    return value;
}

uint8_t fileMgr_readU8(file_t *file)
{
    uint8_t value;
    file->read(value);
    return value;
}

uint16_t fileMgr_readU16(file_t *file)
{
    uint16_t value;
    file->read(value);
    return value;
}

uint32_t fileMgr_readU32(file_t *file)
{
    uint32_t value;
    file->read(value);
    return value;
}

float fileMgr_readFloat(file_t *file)
{
	float value;
    file->read(value);
    return value;
}

void fileMgr_readData(file_t *file, void *data, int32_t len)
{
    file->read((char *)data,len);
}


int8_t *fileMgr_readLine(file_t *file)
{
	// todo: optimize this..
    uint32_t currentSeek =fileMgr_getSeek(file);
    long fileSize = file->size();
    long maxLen = fileSize - currentSeek;
	if( maxLen == 0 )
		return NULL; // eof reached
	// begin parsing
    int8_t *cstr = (int8_t*)malloc(512);
    int32_t size = 0;
    int32_t limit = 512;
	while( maxLen )
	{
		maxLen--;
        int8_t n = fileMgr_readS8(file);
		if( n == '\r' )
			continue; // skip
		if( n == '\n' )
			break; // line end
		cstr[size] = n;
		size++;
		if( size == limit )
            assert(false);
	}
	cstr[size] = '\0';
	return cstr;
}

void fileMgr_setSeek(file_t *file, uint32_t seek)
{
#ifdef WIN32
	SetFilePointer(file->hFile, seek, NULL, FILE_BEGIN);
#else
    fseek(file->hFile,seek,SEEK_SET);
#endif
}

uint32_t fileMgr_getSeek(file_t *file)
{
#ifdef WIN32
    return SetFilePointer(file->hFile, 0, NULL, FILE_CURRENT);
#else
    return ftell(file->hFile);
#endif
}

uint32_t fileMgr_getSize(file_t *file)
{
    return file->size();
}
