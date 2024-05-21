// RmkDelocalise.cpp : Defines the entry point for the application.
//

#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define UNICODE
#define _UNICODE

#if WIN32
#include <Windows.h>
#endif

#ifdef _MSC_VER
#include "getopt.h"
#else
#include <unistd.h>
#endif

#include "RmkDelocalise.h"
#include <algorithm>
#include <filesystem>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>
#include <cassert>
#include <lcf/ldb/reader.h>
#include <lcf/lmt/reader.h>
#include <lcf/lmu/reader.h>
#include <lcf/rpg/moveroute.h>
#include <lcf/reader_util.h>
#include <lcf/writer_lcf.h>
#include <unordered_map>
#include <filesystem>
#include <sys/stat.h>
#include <sys/utime.h>




extern "C" {
#include "md5.h"
}

void NullFileTime(std::string& file) { //semi working doesn't work w folders
#if WIN32

	int wstrlen = MultiByteToWideChar(CP_UTF8, 0, file.c_str(), -1, NULL, 0);
	wchar_t* wstr = new wchar_t[wstrlen];
	MultiByteToWideChar(CP_UTF8, 0, file.c_str(), -1, wstr, wstrlen);

	//"Creates or opens a file or I/O device.
	// The most commonly used I/O devices are as follows:
	// file, file stream, **directory**,
	// physical disk, volume, console buffer,
	// tape drive, communications resource, mailslot, and pipe."
	// ^^^^^^^^ fat fucking lie it does not open directories
	HANDLE hFile = CreateFileW(wstr, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_WRITE_ATTRIBUTES, NULL);
	delete[] wstr;
	if (hFile == INVALID_HANDLE_VALUE) {
		std::cerr << "Could not open file to set time?!?!?!?!" << std::endl; exit(-1);
	}
	FILE_BASIC_INFO b = {};
	//apparently if i leave it at zero it does nothing, so 1 it'll be
	b.LastWriteTime.QuadPart = 1ll;
	b.ChangeTime.QuadPart = 1ll;
	b.CreationTime.QuadPart = 1ll;
	b.LastAccessTime.QuadPart = 1ll;
	BY_HANDLE_FILE_INFORMATION bb;
	GetFileInformationByHandle(hFile, &bb);
	b.FileAttributes = bb.dwFileAttributes;
	SetFileInformationByHandle(hFile, FileBasicInfo, &b, sizeof(b));
	CloseHandle(hFile);
#else
	struct utimbuf t;
	t.actime = 0;
	t.modtime = 0;
	utime(file.c_str(), &t);
#endif
}

std::string md5tostr(uint8_t* digest) {
	//reuse buffers because i can
	static char str[16 * 2 + 1] = {};
	static char buf[2 + 1] = {};
	str[0] = 0;

	for (unsigned int i = 0; i < 16; ++i) {
		sprintf(buf, "%02x", digest[i]);
		strcat(str, buf);
	}
	return std::string(str, 32);
}

std::string StringsMd5(std::vector<std::string>& strs) {
	MD5Context ctx = {};
	md5Init(&ctx);
	for (auto str : strs) {
		md5Update(&ctx, (uint8_t*)str.c_str(), str.length());
	}
	md5Finalize(&ctx);
	return md5tostr(ctx.digest);
}

const char* BACKDROP = "Backdrop";
const char* BATTLE = "Battle";
const char* BATTLE2 = "Battle2";
const char* BATTLECS = "BattleCharSet";
const char* BATTLEWP = "BattleWeapon";
const char* CHARSET = "CharSet";
const char* CHIPSET = "ChipSet";
const char* FACESET = "FaceSet";
const char* FRAME = "Frame";
const char* GOVER = "GameOver";
const char* MONSTER = "Monster";
const char* MOVIE = "Movie";
const char* MUSIC = "Music";
const char* PANORAMA = "Panorama";
const char* PICTURE = "Picture";
const char* SOUND = "Sound";
const char* SYSTEM = "System";
const char* SYSTEM2 = "System2";
const char* TITLE = "Title";

const char* FOLDERLIST[] = {
	BACKDROP,
	BATTLE,
	BATTLE2 ,
	BATTLECS,
	BATTLEWP,
	CHARSET ,
	CHIPSET ,
	FACESET ,
	FRAME,
	GOVER,
	MONSTER ,
	MOVIE,
	MUSIC,
	PANORAMA,
	PICTURE ,
	SOUND,
	SYSTEM,
	SYSTEM2 ,
	TITLE ,
};

const char* NOTFOUND = "Deloc_FileNotExist";
//Extensions can be in upper/lowercase so this won't work with linux but who the fuck gives a shit
const char* EXTS[] = {
	".png",
	".xyz",
	".wav",
	".mp3",
	".mid",
	".midi",
	".bmp",
	".avi"
};

//List of commands mapping which will have a string that wants an asset
const std::unordered_map<lcf::rpg::EventCommand::Code, const char*> gParamDest{
	{lcf::rpg::EventCommand::Code::ChangeFaceGraphic, FACESET},
	{lcf::rpg::EventCommand::Code::ChangeSpriteAssociation, CHARSET},
	{lcf::rpg::EventCommand::Code::ChangeActorFace, FACESET},
	{lcf::rpg::EventCommand::Code::ChangeVehicleGraphic, CHARSET},
	{lcf::rpg::EventCommand::Code::ChangeSystemBGM, MUSIC},
	{lcf::rpg::EventCommand::Code::ChangeSystemSFX, SOUND},
	{lcf::rpg::EventCommand::Code::ChangeSystemGraphics, SYSTEM},
	{lcf::rpg::EventCommand::Code::ShowPicture, PICTURE},
	//move route will get special handling
	{lcf::rpg::EventCommand::Code::PlayBGM, MUSIC},
	{lcf::rpg::EventCommand::Code::PlaySound, SOUND},
	{lcf::rpg::EventCommand::Code::PlayMovie, MOVIE},
	{lcf::rpg::EventCommand::Code::ChangePBG, PANORAMA},
	{lcf::rpg::EventCommand::Code::ChangeBattleBG, BACKDROP},
	{lcf::rpg::EventCommand::Code::EnemyEncounter, BACKDROP}
	//Not gonna bother with Maniac_
};

//can't devise any better way, cry about it
// 
//				   og file hash
//				  (as a string
//				   because i'm
//				   a lazy fuck)			   folder		og fname	 trans fname
std::unordered_map<std::string, std::tuple<const char*, std::string, std::string>> gAssetMap{};

struct {
	bool hashWhenNotFound = { 0 };
	bool alertNotFound = { 1 };
	bool dryRun = { 0 };
	bool printAssList = { 0 };
	bool printAssReverse = { 0 };
	bool removeDates = { 0 };
} gOpt;

char* gStatusStack[10] = {};
char** gStatusPtr = gStatusStack;

char gStatusAccum[10][100] = {};

std::string gEncoding = "";// = "ibm-943_P15A-2003";
//std::string gGameDir = std::string(u8"D:\\DocumentsFolder\\Yume nikki stuff\\ゆめにっき\\ゆめにっき0.10");
std::string gGameDir = "";
std::string gTranslateDir;

lcf::EngineVersion gEngineVer;

inline void PushStatus(char* status) {
	*gStatusPtr++ = status;
}
inline void PopStatus() {
	--gStatusPtr;
}
void PrintStatus() {
	for (char** ptr = gStatusStack; ptr < gStatusPtr; ptr++) {
		std::cout << *ptr;
	}
}

//Please tell me a better way to do this
std::string AssetMapToString() {
	std::unordered_map<const char*, std::vector<std::tuple<std::string, std::string>>> map;
	for (auto i : FOLDERLIST) { //create vectors
		map[i] = std::vector<std::tuple<std::string, std::string>>();
	}
	for (auto& entry : gAssetMap) {
		map[std::get<0>(entry.second)].push_back(std::tuple<std::string, std::string>(
			std::get<1>(entry.second),
			std::get<2>(entry.second)
		));
	}
	std::stringstream str;
	for (auto& entry : map) {
		str << ">" << entry.first << std::endl;
		for (auto& i : entry.second) {
			if(gOpt.printAssReverse)
				str << std::get<1>(i) << "|" << std::get<0>(i) << std::endl;
			else
				str << std::get<0>(i) << "|" << std::get<1>(i) << std::endl;
		}
	}
	return str.str();
}

//I'm not the smartest
const char* FindActualFolderPtr(std::string str) { for (auto i : FOLDERLIST) if (str == i) return i; }

void FileToAssetMap(std::ifstream& f) { //untested
	const char* folder = "youstupid";
	std::string line;
	while (!std::getline(f, line).eof()) { //*might* require last line in file to be empty newline idfk
		if (line[0] == '>') {
			folder = FindActualFolderPtr(line.substr(1));
		}
		else {
			auto pipePos = line.find('|');
			auto ogFile = line.substr(0, pipePos);
			auto transFile = line.substr(pipePos + 1);
			auto checksum = StringsMd5(std::vector<std::string> { folder, ogFile.substr(0, ogFile.find_last_of('.')) });
			gAssetMap[checksum] = std::tuple<const char*, std::string, std::string>(folder, ogFile, transFile);
		}
	}
}

//There doesn't appear to be some built in thing in liblcf for interpreting move routes
//so I have to yoink code from Player; these were found in game_interpreter.cpp
#pragma region Move route related: DecodeMove, DecodeInt, DecodeString..
int DecodeInt(lcf::DBArray<int32_t>::const_iterator& it) {
	int value = 0;

	for (;;) {
		int x = *it++;
		value <<= 7;
		value |= x & 0x7F;
		if (!(x & 0x80))
			break;
	}

	return value;
}

//i hope to god this is correct, this at least fixes the closet sound effects in the guillotine world
void EncodeInt(int val, std::vector<int32_t>& codes) {
	uint32_t value = (uint32_t)val;
	//char buffer[4];
	//char* bufferPtr = buffer;
	for (int i = 28; i >= 0; i -= 7)
		if (value >= (1U << i) || i == 0)
			codes.push_back ((uint8_t)(((value >> i) & 0x7F) | (i > 0 ? 0x80 : 0)));
	//codes.push_back(*reinterpret_cast<int32_t*>(buffer));
}

lcf::DBString DecodeString(lcf::DBArray<int32_t>::const_iterator& it) {
	int len = DecodeInt(it);
	char out[MAX_PATH] = {};

	for (int i = 0; i < len; i++)
		out[i] = (char)*it++;

	//return lcf::DBString("fuckup");
	std::string result = lcf::ReaderUtil::Recode(lcf::StringView(out, len), gEncoding);
	return lcf::DBString(result);
}

void EncodeString(lcf::DBString& dbstr, std::vector<int32_t>& codes) {
	auto str = lcf::ToString(dbstr);
	static lcf::Encoder e(gEncoding);
	e.Decode(str);

	EncodeInt(strlen(str.data()), codes);
	for (unsigned char c : str) {
		codes.push_back((uint32_t)c);
	}
	//codes.insert(codes.end(), str.begin(), str.end());
}

lcf::rpg::MoveCommand DecodeMove(lcf::DBArray<int32_t>::const_iterator& it) {
	lcf::rpg::MoveCommand cmd;
	cmd.command_id = *it++;

	switch (cmd.command_id) {
	case 32:	// Switch ON
	case 33:	// Switch OFF
		cmd.parameter_a = DecodeInt(it);
		break;
	case 34:	// Change Graphic
		cmd.parameter_string = lcf::DBString(DecodeString(it));
		cmd.parameter_a = DecodeInt(it);
		break;
	case 35:	// Play Sound Effect
		cmd.parameter_string = lcf::DBString(DecodeString(it));
		cmd.parameter_a = DecodeInt(it);
		cmd.parameter_b = DecodeInt(it);
		cmd.parameter_c = DecodeInt(it);
		break;
	}

	return cmd;
}
lcf::rpg::MoveRoute CommandParamsToMoveRoute(lcf::DBArray<int32_t>& params) {
	lcf::rpg::MoveRoute moveRoute;
	moveRoute.repeat = params[2] != 0;
	moveRoute.skippable = params[3] != 0;
	for (lcf::DBArray<int32_t>::const_iterator it = params.begin() + 4; it < params.end();) {
		moveRoute.move_commands.push_back(DecodeMove(it));
	}
	return moveRoute;
}
lcf::DBArray<int32_t> MoveRouteToCommandParams(lcf::rpg::MoveRoute& moveRoute, int target, int moveFreq) {
	std::vector<int32_t> codes;
	codes.push_back(target);
	codes.push_back(moveFreq);
	codes.push_back(moveRoute.repeat ? 1 : 0);
	codes.push_back(moveRoute.skippable ? 1 : 0);
	for (auto& cmd : moveRoute.move_commands) {
		codes.push_back(cmd.command_id);
		switch (cmd.command_id) {
		case 32:
		case 33:
			EncodeInt(cmd.parameter_a, codes);
			break;
		case 34:
			EncodeString(cmd.parameter_string, codes);
			EncodeInt(cmd.parameter_a, codes);
			break;
		case 35:
			EncodeString(cmd.parameter_string, codes);
			EncodeInt(cmd.parameter_a, codes);
			EncodeInt(cmd.parameter_b, codes);
			EncodeInt(cmd.parameter_c, codes);
			break;
		}
	}
	lcf::DBArray<int32_t> params(codes.begin(), codes.end());
	return params;
}
#pragma endregion


#define DOASSET(_folder, _asset) \
PushStatus(#_asset); \
ProcessAsset(_folder, _asset); \
PopStatus();

//For some reason liblcf has a fuckass class for sounds that does
//NOT utilise dbstrings.
lcf::DBString gSoundFix;
#define DOSFX(_folder, _member) \
PushStatus(#_member); \
gSoundFix = lcf::DBString(_member); \
ProcessAsset(_folder, gSoundFix); \
_member = lcf::ToString(gSoundFix); \
PopStatus();

const char* FindAssetExt(const char* dir, lcf::DBString& name) {
	std::ifstream ifs;
	std::string path = gGameDir + "/" + dir + "/" + lcf::ToString(name);
	for (auto ext : EXTS) {
		ifs.open(path + ext);
		if (!ifs.fail()) {
			ifs.close();
			return ext;
		}
	}
	if (gOpt.alertNotFound) {
		PrintStatus();
		std::cout << " FILE NOT FOUND: " << dir << "/" << lcf::ToString(name) << std::endl;
	}
	return NOTFOUND;
}

void ProcessAsset(const char* folder, lcf::DBString& name) {
	std::string namestr = lcf::ToString(name);
	//2kki does some funky things with directory traversing, most notably with pc_back.png
	//i don't know how to deal with that. let's just hope the fabric of reality doesn't collapse onto itself.
	if (namestr == "(OFF)" || namestr.empty() /* || namestr.find("..") == 0*/) return;

	auto checksum = StringsMd5(std::vector<std::string> { folder, namestr });

	auto found = gAssetMap.find(checksum);
	if (found == gAssetMap.end()) { //not in map
		const char* ext = FindAssetExt(folder, name);
		if (ext == NOTFOUND) return; //do nothing if there's no file to begin with
		std::string finalName = gOpt.hashWhenNotFound ? checksum : namestr;
		gAssetMap[checksum] = std::tuple<const char*, std::string, std::string>(folder, namestr + ext, finalName + ext);
		name = lcf::DBString(finalName);
	}
	else {
		auto& transName = std::get<2>(found->second);
		name = lcf::DBString(transName.substr(0, transName.find_last_of('.')));
	}
}

void DoMoveRoute(lcf::rpg::MoveRoute& moveRoute) {
	for (auto& move : moveRoute.move_commands) {
		switch (move.command_id) {
		case 34:
			DOASSET(CHARSET, move.parameter_string);
			break;
		case 35:
			DOASSET(SOUND, move.parameter_string);
			break;
		}
	}
}

void DoCommandList(std::vector<lcf::rpg::EventCommand>& list) {
	for (auto& cmd : list) {
		sprintf(gStatusAccum[1], "Do command %s/", lcf::rpg::EventCommand::kCodeTags.tag(cmd.code));
		PushStatus(gStatusAccum[1]);
		auto code = (lcf::rpg::EventCommand::Code)cmd.code;
		if (code == lcf::rpg::EventCommand::Code::MoveEvent) {
			auto moveRoute = CommandParamsToMoveRoute(cmd.parameters);
			DoMoveRoute(moveRoute);
			cmd.parameters = MoveRouteToCommandParams(moveRoute, cmd.parameters[0], cmd.parameters[1]);
		}
		auto found = gParamDest.find(code);
		if (found != gParamDest.end()) {
			auto dir = found->second;
			DOASSET(dir, cmd.string);
		}
		PopStatus();
	}
}

void DoLdb(lcf::rpg::Database* db) {
	gEngineVer = db->system.ldb_id == 2003 ? lcf::EngineVersion::e2k3 : lcf::EngineVersion::e2k;
	PushStatus("Do Database/");
	for (auto& actor : db->actors) {
		sprintf(gStatusAccum[0], "Do actor %04d:%s/", actor.ID, actor.name.c_str());
		PushStatus(gStatusAccum[0]);
		DOASSET(FACESET, actor.face_name);
		DOASSET(CHARSET, actor.character_name);
		PopStatus();
	}
	for (auto& enemy : db->enemies) {
		sprintf(gStatusAccum[0], "Do monster %04d:%s/", enemy.ID, enemy.name.c_str());
		PushStatus(gStatusAccum[0]);
		DOASSET(MONSTER, enemy.battler_name);
		PopStatus();
	}
	for (auto& troop : db->troops) {
		sprintf(gStatusAccum[0], "Do troop %04d:%s/", troop.ID, troop.name.c_str());
		PushStatus(gStatusAccum[0]);
		for (auto& page : troop.pages) {
			sprintf(gStatusAccum[2], "Do troop page %04d/", page.ID);
			PushStatus(gStatusAccum[2]);
			DoCommandList(page.event_commands);
			PopStatus();
		}
		PopStatus();
	}
	for (auto& anim : db->animations) {
		sprintf(gStatusAccum[0], "Do anim %04d:%s/", anim.ID, anim.name.c_str());
		PushStatus(gStatusAccum[0]);
		for(auto& timing : anim.timings) {
			sprintf(gStatusAccum[2], "Do timing %04d/", timing.ID);
			PushStatus(gStatusAccum[2]);
			DOSFX(SOUND, timing.se.name);
			PopStatus();
		}
		if (anim.large) {
			DOASSET(BATTLE2, anim.animation_name);
		}
		else {
			DOASSET(BATTLE, anim.animation_name);
		}
		PopStatus();
	}
	for (auto& anim2 : db->battleranimations) {
		sprintf(gStatusAccum[0], "Do anim2 %04d:%s/", anim2.ID, anim2.name.c_str());
		PushStatus(gStatusAccum[0]);
		for (auto& pose : anim2.poses) {
			sprintf(gStatusAccum[2], "Do pose %04d/", pose.ID);
			PushStatus(gStatusAccum[2]);
			DOASSET(BATTLECS, pose.battler_name);
			PopStatus();
		}
		for (auto& weapon : anim2.weapons) {
			sprintf(gStatusAccum[2], "Do weapon %04d/", weapon.ID);
			PushStatus(gStatusAccum[2]);
			DOASSET(BATTLEWP, weapon.weapon_name);
			PopStatus();
		}
		PopStatus();
	}
	for (auto& terrain : db->terrains) {
		sprintf(gStatusAccum[0], "Do terrain %04d:%s/", terrain.ID, terrain.name.c_str());
		PushStatus(gStatusAccum[0]);
		DOASSET(FRAME, terrain.background_a_name);
		DOASSET(FRAME, terrain.background_b_name);
		DOASSET(BACKDROP, terrain.background_name);
		DOSFX(SOUND,terrain.footstep.name);
		PopStatus();
	}
	for (auto& chipset : db->chipsets) {
		sprintf(gStatusAccum[0], "Do chipset %04d:%s/", chipset.ID, chipset.name.c_str());
		PushStatus(gStatusAccum[0]);
		DOASSET(CHIPSET, chipset.chipset_name);
		PopStatus();
	}
	//system stuff
	DOASSET(CHARSET, db->system.boat_name);
	DOASSET(CHARSET, db->system.ship_name);
	DOASSET(CHARSET, db->system.airship_name);
	DOASSET(TITLE, db->system.title_name);
	DOASSET(GOVER, db->system.gameover_name);
	DOASSET(SYSTEM, db->system.system_name);
	DOASSET(SYSTEM2, db->system.system2_name);

	DOSFX(MUSIC, db->system.title_music.name);
	DOSFX(MUSIC, db->system.battle_music.name);
	DOSFX(MUSIC, db->system.battle_end_music.name);
	DOSFX(MUSIC, db->system.inn_music.name);
	DOSFX(MUSIC, db->system.boat_music.name);
	DOSFX(MUSIC, db->system.ship_music.name);
	DOSFX(MUSIC, db->system.airship_music.name);
	DOSFX(MUSIC, db->system.gameover_music.name);

	DOSFX(SOUND, db->system.cursor_se.name);
	DOSFX(SOUND, db->system.decision_se.name);
	DOSFX(SOUND, db->system.cancel_se.name);
	DOSFX(SOUND, db->system.buzzer_se.name);
	DOSFX(SOUND, db->system.battle_se.name);
	DOSFX(SOUND, db->system.escape_se.name);
	DOSFX(SOUND, db->system.enemy_attack_se.name);
	DOSFX(SOUND, db->system.enemy_damaged_se.name);
	DOSFX(SOUND, db->system.actor_damaged_se.name);
	DOSFX(SOUND, db->system.dodge_se.name);
	DOSFX(SOUND, db->system.enemy_death_se.name);
	DOSFX(SOUND, db->system.item_se.name);

	DOASSET(BACKDROP, db->system.battletest_background);
	DOASSET(FRAME, db->system.frame_name);

	for (auto& ce : db->commonevents) {
		sprintf(gStatusAccum[0], "Do common event %04d:%s/", ce.ID, ce.name.c_str());
		PushStatus(gStatusAccum[0]);
		DoCommandList(ce.event_commands);
		PopStatus();
	}
	if (!gOpt.dryRun) {
		std::string accum;
		lcf::LDB_Reader::Save(accum = (gTranslateDir + "/RPG_RT.ldb"), *db, gEncoding);
		if (gOpt.removeDates) NullFileTime(accum);
	}
	PopStatus();
}

void DoLmu(lcf::rpg::Map* lmu) {
	DOASSET(PANORAMA, lmu->parallax_name);
	for (auto& event : lmu->events) {
		for (auto& page : event.pages) {
			sprintf(gStatusAccum[2], "Do event %04d:%s PAGE %d/", event.ID, event.name.c_str(), page.ID);
			PushStatus(gStatusAccum[2]);
			DOASSET(CHARSET, page.character_name);
			DoMoveRoute(page.move_route);
			DoCommandList(page.event_commands);
			PopStatus();
		}
	}
}

void DoLmt(lcf::rpg::TreeMap* lmt) {
	std::string accum;
	PushStatus("Do maps/");
	for (auto& mapInfo : lmt->maps) {
		if (mapInfo.ID == 0) continue; //id 0 is the game title for some reason
		sprintf(gStatusAccum[0], "Do map %04d:%s/", mapInfo.ID, mapInfo.name.c_str());
		PushStatus(gStatusAccum[0]);
		DOASSET(BACKDROP, mapInfo.background_name);
		DOSFX(MUSIC, mapInfo.music.name);

		char mapname[13];
		sprintf(mapname, "/Map%04d.lmu", mapInfo.ID);
		auto lmu = lcf::LMU_Reader::Load(gGameDir + mapname, gEncoding);
		if (lmu) { //2kki lmt references deleted maps
			DoLmu(lmu.get());
			if (!gOpt.dryRun) {
				lcf::LMU_Reader::Save(accum = (gTranslateDir + mapname), *lmu, gEngineVer, gEncoding);
				if (gOpt.removeDates) NullFileTime(accum);
			}
		}
		PopStatus();
	}
	if (!gOpt.dryRun) {
		lcf::LMT_Reader::Save(accum = (gTranslateDir + "/RPG_RT.lmt"), *lmt, gEngineVer, gEncoding);
		if (gOpt.removeDates) NullFileTime(accum);
	}
	PopStatus();
}

void PrintUsage(char* a) {
	printf(
		"USAGE: %s [switches] /game/directory\n"
		"-d		Enables dry run (does not write any files).\n"
		"		Using only this switch is useful to find references to\n"
		"		deleted game resources.\n"
		"\n"
		"-h		If an asset is found to not be present in the asset map,\n"
		"		insert a hash of the asset name into the asset map.\n"
		"		Otherwise, do not change the asset name.\n"
		"\n"
		"-u		DO NOT warn about assets that do not exist.\n" //"unused"
		"\n"
		"-m		Provide a file that describes the asset map.\n"
		"		EXAMPLE: -p ./assetmap.txt\n"
		"\n"
		"-M		Print the asset map after execution.\n"
		"\n"
		"-R		Print the asset map but invert the positions of the\n"
		"		original file and the translated file.\n"
		"\n"
		//"-t		Redact timestamps from the translated files.\n" //barely works
		//"\n"
		"-c		Specify the codepage to use when reading game text.\n"
		"		Default is 932 (shift-jis).\n"
		"		EXAMPLE: -c 932\n"
		"\n"
		"-C		Specify the codepage to use when reading arguments\n"
		"		given to this program. Default is the same as the\n"
		"		game text codepage (shift-jis). MUST BE BEFORE ANY\n"
		"		OTHER ARGUMENTS.\n"
		"		EXAMPLE: -C 932\n", a
	);
}

int main(int argc, char** argv)
{
#if WIN32
	SetConsoleOutputCP(CP_UTF8); //set console to utf8 
#endif
	setlocale(LC_ALL, "en_US.UTF-8");
	gEncoding = lcf::ReaderUtil::CodepageToEncoding(932);
	std::string argEncoding = "";

	opterr = 0;
	char c = 0;
	//pissed off at my inconsistent usage of stl and c functions? GOOD
	while ((c = getopt(argc, argv, "dhum:MRtc:C:")) != -1) { 
		switch (c) {
		case 'd':
			gOpt.dryRun = true;
			break;
		case 'h':
			gOpt.hashWhenNotFound = true;
			break;
		case 'u':
			gOpt.alertNotFound = false;
			break;
		case 'm':
			{
				std::ifstream f(lcf::ReaderUtil::Recode(optarg, (argEncoding.empty())
					? gEncoding
					: argEncoding));
				if (f.bad()) {
					std::cerr << "Could not open asset map file" << std::endl; exit(-1);
				}
				FileToAssetMap(f);
				f.close();
			}
			break;
		case 'M':
			gOpt.printAssList = true;
			break;
		case 'R':
			gOpt.printAssList = true;
			gOpt.printAssReverse = true;
			break;
		case 't':
			gOpt.removeDates = true;
			break;
		case 'c':
			gEncoding = lcf::ReaderUtil::CodepageToEncoding(atoi(optarg));
			break;
		case 'C':
			argEncoding = lcf::ReaderUtil::CodepageToEncoding(atoi(optarg));
			break;
		case '?':
			std::cerr << "Blah blah blah error" << std::endl;
			abort();
			break;
		default:
			PrintUsage(argv[0]); exit(-1);
			break;
		}
	}

	for (int i = optind; i < argc; i++) {
		gGameDir = lcf::ReaderUtil::Recode(argv[i], (argEncoding.empty())
			? gEncoding
			: argEncoding);
	}

	if (gGameDir.empty()) {
		std::cerr << "No game directory specified" << std::endl;
		PrintUsage(argv[0]); exit(-1);
	}
	else if (!std::filesystem::exists(gGameDir)) {
		std::cerr << "Specified game directory does not exist" << std::endl;
		PrintUsage(argv[0]); exit(-1);
	}
	
	gTranslateDir = gGameDir + "/" + "TRANSLATED";
	if (!gOpt.dryRun) {
		if (!std::filesystem::exists(gTranslateDir)) {
			std::filesystem::create_directory(gTranslateDir);
			if (gOpt.removeDates) NullFileTime(gTranslateDir);
			for (auto dir : FOLDERLIST) {
				std::string accum;
				std::filesystem::create_directory(accum = (gTranslateDir + "/" + dir));
				if (gOpt.removeDates) NullFileTime(accum);
			}
		}
		else {
			std::cerr << "Conversion directory already exists; enabling dry run flag" << std::endl;
			gOpt.dryRun = true;
		}
	}

	auto db = lcf::LDB_Reader::Load(gGameDir + "/RPG_RT.ldb", gEncoding);
	if(db) DoLdb(db.get());
	
	auto lmt = lcf::LMT_Reader::Load(gGameDir + "/RPG_RT.lmt", gEncoding);
	if(lmt) DoLmt(lmt.get());

	if (gOpt.printAssList) {
		std::cout << AssetMapToString() << std::endl;
	}

	if (!gOpt.dryRun) {
		std::string accum;
		std::filesystem::copy(gGameDir + "/RPG_RT.ini", accum = (gTranslateDir + "/RPG_RT.ini")); 
		std::filesystem::copy(gGameDir + "/RPG_RT.exe", accum = (gTranslateDir + "/RPG_RT.exe")); 
		for (auto& entry : gAssetMap) {
			auto& tuple = entry.second;
			auto outFile = gTranslateDir + "/" + std::get<0>(tuple) + "/" + std::get<2>(tuple);
			//yes, i have to handle this because of 2kki directory traversing that reutilises files
			if (!std::filesystem::exists(outFile)) {
				std::filesystem::copy(
					gGameDir + "/" + std::get<0>(tuple) + "/" + std::get<1>(tuple),
					outFile);
			}
			if (gOpt.removeDates) {
				NullFileTime(outFile);
			}
		}
	}

	return 0;
}
