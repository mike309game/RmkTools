#ifdef _WIN32
//#pragma message "is windows"
#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define UNICODE
#define _UNICODE
#include <Windows.h>
#else
#define MAX_PATH (260)
#endif

#include <lcf/reader_util.h>
#include <lcf/reader_lcf.h>
#include <lcf/rpg/event.h>
#include <lcf/ldb/reader.h>
#include <lcf/lmt/reader.h>
#include <lcf/lmu/reader.h>

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <utime.h>

typedef struct _opt {
	bool hashWhenNotFound = { 0 };
	bool alertNotFound = { 1 };
	bool dryRun = { 0 };
	bool printAssList = { 0 };
	bool loadAssReverse = { 0 };
	bool removeDates = { 0 };
	bool alertDupeFiles = { 1 };
	//bool alertInvalidFiles = { 1 };
} opt;

// uhhhh

extern const char* BACKDROP;
extern const char* BATTLE;
extern const char* BATTLE2;
extern const char* BATTLECS;
extern const char* BATTLEWP;
extern const char* CHARSET;
extern const char* CHIPSET;
extern const char* FACESET;
extern const char* FRAME;
extern const char* GOVER;
extern const char* MONSTER;
extern const char* MOVIE;
extern const char* MUSIC;
extern const char* PANORAMA;
extern const char* PICTURE;
extern const char* SOUND;
extern const char* SYSTEM;
extern const char* SYSTEM2;
extern const char* TITLE;

extern const char* FOLDERLIST[];

extern opt gOpt;
static const char* NOTFOUND = "Deloc_FileNotExist";
std::string FindAssetExt(const char* kind, std::string name);
void PushStatus(const char* status, ...);
void PrintStatus();
void PopStatus();
void NullDirectoryTimes(std::string_view dir);