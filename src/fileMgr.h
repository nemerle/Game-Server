#pragma once
#include <stdint.h>

struct file_t;

file_t *fileMgr_open(const char * name);
file_t *fileMgr_create(const char * name);
void fileMgr_close(file_t *file);

void fileMgr_writeS8(file_t *file, int8_t value);
void fileMgr_writeS16(file_t *file, int16_t value);
void fileMgr_writeS32(file_t *file, int32_t value);
void fileMgr_writeU8(file_t *file, uint8_t value);
void fileMgr_writeU16(file_t *file, uint16_t value);
void fileMgr_writeU32(file_t *file, uint32_t value);
void fileMgr_writeFloat(file_t *file, float value);
void fileMgr_writeData(file_t *file, void *data, int32_t len);

int8_t fileMgr_readS8(file_t *file);
int16_t fileMgr_readS16(file_t *file);
int32_t fileMgr_readS32(file_t *file);
uint8_t fileMgr_readU8(file_t *file);
uint16_t fileMgr_readU16(file_t *file);
uint32_t fileMgr_readU32(file_t *file);
float fileMgr_readFloat(file_t *file);
void fileMgr_readData(file_t *file, void *data, int32_t len);

int8_t * fileMgr_readLine(file_t *file);

void fileMgr_setSeek(file_t *file, uint32_t seek);
uint32_t fileMgr_getSeek(file_t *file);
uint32 fileMgr_getSize(file_t *file);

// fileMgr extensions
#include "sData.h"
