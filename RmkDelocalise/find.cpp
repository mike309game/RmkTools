#include "RmkDelocalise.hh"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <ostream>
#include <unicode/umachine.h>
#include <unicode/urename.h>
#include <unicode/utypes.h>
#include <unordered_map>
#include <unicode/ustring.h>

static std::unordered_map<std::string, std::unordered_map<std::string, std::string>> folders;

// fun
#define DEF(f, ...) {f, (const char*[]){__VA_ARGS__, nullptr}}
// in the order rpg_rt attempts to load
static std::unordered_map<const char*, const char**> validDirFiles {
	DEF(BACKDROP, ".bmp", ".png", ".xyz"),
	DEF(BATTLE, ".bmp", ".png", ".xyz"),
	DEF(BATTLE2, ".bmp", ".png", ".xyz"),
	DEF(BATTLECS, ".bmp", ".png", ".xyz"),
	DEF(BATTLEWP, ".bmp", ".png", ".xyz"),
	DEF(CHARSET, ".bmp", ".png", ".xyz"),
	DEF(CHIPSET, ".bmp", ".png", ".xyz"),
	DEF(FACESET, ".bmp", ".png", ".xyz"),
	DEF(FRAME, ".bmp", ".png", ".xyz"),
	DEF(GOVER, ".bmp", ".png", ".xyz"),
	DEF(MONSTER, ".bmp", ".png", ".xyz"),
	DEF(MOVIE, ".avi", ".mpg"),
	DEF(MUSIC, ".mid", ".wav", ".mp3"), // huh, no .midi?
	DEF(PANORAMA, ".bmp", ".png", ".xyz"),
	DEF(PICTURE, ".bmp", ".png", ".xyz"),
	DEF(SOUND, ".wav"),
	DEF(SYSTEM, ".bmp", ".png", ".xyz"),
	DEF(SYSTEM2, ".bmp", ".png", ".xyz"),
};

static void Lower(std::string& str) {
	// fuck this cancer

	// If i don't initialise these icu just fucking dies
	UErrorCode hush = {};
	int len = {};
	UChar whatawaste[MAX_PATH] = {};
	//puts(str.c_str());
	u_strFromUTF8Lenient(whatawaste, MAX_PATH, &len, str.data(), str.length(), &hush); assert(!U_FAILURE(hush)); //printf("%s %d\n", u_errorName(hush), hush); 
	u_strToLower(whatawaste, MAX_PATH, whatawaste, len, "", &hush); assert(!U_FAILURE(hush));
	u_strToUTF8(str.data(), str.length()+1, &len, whatawaste, len, &hush); assert(!U_FAILURE(hush));
}

extern std::string gGameDir;

std::string FindAssetExt(const char* kind, std::string name) {
	PushStatus("/Find in \"%s\" file \"%s\"/", kind, name.c_str());
	try {
	name = gGameDir + "/" + (kind + ('/' + name));

	// This thing doesnt seem to deal with backslashes on godbolt (it
	// was treating backslashes as part of the file name) and I have
	// no idea if it's os dependent. i don't know how you make something
	// that isn't at least as good as System.IO.Path. BY the way it's
	// very great and very convenient that strings don't have a replace
	// characters function built in
	std::replace(name.begin(), name.end(), '\\', '/');

	std::filesystem::path path(name);

	// we pray that any possible directory traversing will have the same
	// casing as the folder present in the files

	// UPDATE: ゆめ2っき/Sound/../music
	// i really really don't feel like writing code to open case insensitive directories
	// just so that peepee poopoo OS can run this program
	auto dir = path.parent_path().string();

	auto files = folders.find(dir);
	while(files == folders.end()) { // didn't cache this dir's files yet
		// LMT/Map[1871] MAP1871 qxy home/Event[0001] 開始処理 @ [000,000]/PAGE[01]/Command[418] PlaySound/cmd.string/Find in "Sound" file "..Music\Chihiro_WhiteImage_Loop"/FIND EXT ERROR!!!
		if(!std::filesystem::exists(dir)) {
			if(gOpt.alertNotFound) {
				PrintStatus();
				std::cout << "DIRECTORY DOESNT EXIST!!!" << std::endl;
			}
			PopStatus();
			return NOTFOUND;
		}
		files = folders.emplace(dir, std::unordered_map<std::string, std::string>()).first;
		auto& extMap = files->second;
		for(const auto& entry : std::filesystem::directory_iterator(dir)) {
			if(entry.is_directory())
				continue;
			std::string fname(entry.path().filename().stem().string());
			Lower(fname);
			std::string ext = entry.path().extension().string();
			auto inserted = extMap.emplace(fname, ext);
			if(!inserted.second) { // dupe file
				// check if the dupe file is the file the game actually reads
				for(auto tryext = validDirFiles[kind]; *tryext != nullptr; tryext++) {
					if(strcasecmp(ext.c_str(), *tryext) == 0) { // Prioritise the new file
						inserted.first->second = ext; break;
					} else if(strcasecmp(inserted.first->second.c_str(), *tryext) == 0) { // Prioritise existing file
						break; // no need to change
					}
				}
				if(gOpt.alertDupeFiles) {
					PrintStatus();
					std::cout << "DIR " << dir << " HAS IDENTICALLY NAMED FILE: " << fname << std::endl;
				}
			}
		}
	}
	auto lowered = path.filename().string();
	Lower(lowered);
	auto ext = files->second.find(lowered);
	if(ext != files->second.end()) { // found ext
		PopStatus();
		return ext->second;
	}
	if(gOpt.alertNotFound) {
		PrintStatus();
		std::cout << "FILE NOT FOUND!!!" << std::endl;
	}
	PopStatus();
	return NOTFOUND;
	} catch(std::exception e) {
		PrintStatus();
		std::cout << "FIND EXT ERROR!!! " << e.what(); exit(-1);
	}
}