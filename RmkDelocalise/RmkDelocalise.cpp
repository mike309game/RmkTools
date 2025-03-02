// RmkDelocalise.cpp : Defines the entry point for the application.
//

#include "RmkDelocalise.hh"
#include "lcf/rpg/movecommand.h"
#include <filesystem>
extern "C" {
#include "md5.h"
}

std::string md5tostr(uint8_t* digest) {
	char str[16 * 2 + 1];
	sprintf(str, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
			digest[0], digest[1], digest[2], digest[3], digest[4], digest[5], digest[6], digest[7],
			digest[8], digest[9], digest[10], digest[11], digest[12], digest[13], digest[14], digest[15]);
	return std::string(str, 32);
}

// when this was msvc, it was std::vector<std::string>& and it worked fine,
// but now gcc is yelling at me???
std::string StringsMd5(std::vector<std::string> strs) {
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

/*//Extensions can be in upper/lowercase so this won't work with linux but who the fuck gives a shit
const char* EXTS[] = {
	".png",
	".xyz",
	".wav",
	".mp3",
	".mid",
	".midi",
	".bmp",
	".avi"
};*/

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
//				  (as a stl string
//				   because i  
//				   dont know  
//				   how a struct
//				   or something
//				   would work) 			   folder		og fname	 trans fname
std::unordered_map<std::string, std::tuple<const char*, std::string, std::string>> gAssetMap{};

opt gOpt;

std::string gEncoding = "";// = "ibm-943_P15A-2003";
//std::string gGameDir = std::string(u8"D:\\DocumentsFolder\\Yume nikki stuff\\ゆめにっき\\ゆめにっき0.10");
std::string gGameDir = "";
std::string gTranslateDir;

lcf::EngineVersion gEngineVer;

char gStatusAccum[0x7fff];
char* gStatusPtr[20] = {gStatusAccum};
int gStatusCount = 0;

void PushStatus(const char* status, ...) {
	va_list va;
	auto ptr = gStatusPtr[gStatusCount];
	va_start(va, status);
	int len = vsnprintf(ptr, &gStatusAccum[sizeof(gStatusAccum)] - ptr, status, va);
	va_end(va);
	gStatusPtr[++gStatusCount] = ptr + len + 1;
}
void PopStatus() {
	gStatusCount--;
}
void PrintStatus() {
	for (int i = 0; i < gStatusCount; ++i)
		std::cout << gStatusPtr[i];
}

//Please tell me a better way to do this
std::string AssetMapToString() {
	std::unordered_map<const char*, std::vector<std::tuple<std::string, std::string>>> map;
	for (auto i : FOLDERLIST) { //create vectorss
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
			str << std::get<0>(i) << "|" << std::get<1>(i) << std::endl;
		}
	}
	return str.str();
}

//I'm not the smartest
const char* FindActualFolderPtr(std::string str) { for (auto i : FOLDERLIST) if (str == i) return i; perror("bad happened"); return NULL;}

void FileToAssetMap(std::ifstream& f) { //untested
	const char* folder = "youstupid";
	std::string line;
	while (!std::getline(f, line).eof()) { //*might* require last line in file to be empty newline idfk
		if(line[0] == '#') continue;
		if (line[0] == '>') {
			folder = FindActualFolderPtr(line.substr(1));
		}
		else {
			auto pipePos = line.find('|');
			auto ogFile = line.substr(0, pipePos);
			auto transFile = line.substr(pipePos + 1);
			auto checksum = StringsMd5(/*std::vector<std::string>*/ { folder, ogFile.substr(0, ogFile.find_last_of('.')) });
			if(gOpt.loadAssReverse)
				gAssetMap[checksum] = std::tuple<const char*, std::string, std::string>(folder, transFile, ogFile);
			else
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
	if(len >= MAX_PATH) {
		std::advance(it, len);
		puts("way too long string");
		return "this string is way too long";
	}
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
		case 32: // Switch ON
		case 33: // Switch OFF
			EncodeInt(cmd.parameter_a, codes);
			break;
		case 34: // Change Graphic
			EncodeString(cmd.parameter_string, codes);
			EncodeInt(cmd.parameter_a, codes);
			break;
		case 35: // Play Sound Effect
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

void ProcessAsset(const char* folder, lcf::DBString& name) {
	std::string namestr = lcf::ToString(name);
	//2kki does some funky things with directory traversing, most notably with pc_back.png
	//i don't know how to deal with that. let's just hope the fabric of reality doesn't collapse onto itself.
	if (namestr == "(OFF)" || namestr.empty() /* || namestr.find("..") == 0*/) return;

	auto checksum = StringsMd5(/*std::vector<std::string>*/ { folder, namestr });

	auto found = gAssetMap.find(checksum);
	if (found == gAssetMap.end()) { //not in map
		auto ext = FindAssetExt(folder, name.c_str());
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
	for (int i = 0; i < moveRoute.move_commands.size(); ++i)
	{
		auto& move = moveRoute.move_commands[i];
		PushStatus("[%d] %s", i, lcf::rpg::MoveCommand::kCodeTags.tag(move.command_id));
		// call ProcessAsset directly to reduce redundancy 
		switch (move.command_id) {
		case 34:
			ProcessAsset(CHARSET, move.parameter_string);
			break;
		case 35:
			ProcessAsset(SOUND, move.parameter_string);
			break;
		}
		PopStatus();
	}
}

void DoCommandList(std::vector<lcf::rpg::EventCommand>& list) {
	for(int i = 0; i < list.size(); i++) {
		auto& cmd = list[i];
		PushStatus("Command[%d] %s/", i, lcf::rpg::EventCommand::kCodeTags.tag(cmd.code));
		auto code = (lcf::rpg::EventCommand::Code)cmd.code;
		if (code == lcf::rpg::EventCommand::Code::MoveEvent) {
			auto moveRoute = CommandParamsToMoveRoute(cmd.parameters);
			DoMoveRoute(moveRoute);
			cmd.parameters = MoveRouteToCommandParams(moveRoute, cmd.parameters[0], cmd.parameters[1]);
		} else {
			auto found = gParamDest.find(code);
			if (found != gParamDest.end()) {
				auto dir = found->second;
				DOASSET(dir, cmd.string);
			}
		}
		PopStatus();
	}
}

void DoLdb(lcf::rpg::Database* db) {
	gEngineVer = db->system.ldb_id == 2003 ? lcf::EngineVersion::e2k3 : lcf::EngineVersion::e2k;
	PushStatus("LDB/");
	for (auto& actor : db->actors) {
		PushStatus("Actor[%04d] %s/", actor.ID, actor.name.c_str());
		DOASSET(FACESET, actor.face_name);
		DOASSET(CHARSET, actor.character_name);
		PopStatus();
	}
	for (auto& enemy : db->enemies) {
		PushStatus("Monster[%04d] %s/", enemy.ID, enemy.name.c_str());
		DOASSET(MONSTER, enemy.battler_name);
		PopStatus();
	}
	for (auto& troop : db->troops) {
		PushStatus("Troop[%04d] %s/", troop.ID, troop.name.c_str());
		for (auto& page : troop.pages) {
			PushStatus("Page[%04d]/", page.ID);
			DoCommandList(page.event_commands);
			PopStatus();
		}
		PopStatus();
	}
	for (auto& anim : db->animations) {
		PushStatus("Anim[%04d] %s/", anim.ID, anim.name.c_str());
		for(auto& timing : anim.timings) {
			PushStatus("Timing[%04d]/", timing.ID);
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
		PushStatus("Anim2[%04d] %s/", anim2.ID, anim2.name.c_str());
		for (auto& pose : anim2.poses) {
			PushStatus("Pose [%04d]/", pose.ID);
			DOASSET(BATTLECS, pose.battler_name);
			PopStatus();
		}
		for (auto& weapon : anim2.weapons) {
			PushStatus("Weapon[%04d]/", weapon.ID);
			DOASSET(BATTLEWP, weapon.weapon_name);
			PopStatus();
		}
		PopStatus();
	}
	for (auto& terrain : db->terrains) {
		PushStatus("Terrain[%04d] %s/", terrain.ID, terrain.name.c_str());
		DOASSET(FRAME, terrain.background_a_name);
		DOASSET(FRAME, terrain.background_b_name);
		DOASSET(BACKDROP, terrain.background_name);
		DOSFX(SOUND,terrain.footstep.name);
		PopStatus();
	}
	for (auto& chipset : db->chipsets) {
		PushStatus("Chipset[%04d] %s/", chipset.ID, chipset.name.c_str());
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
		PushStatus("Common event[%04d] %s/", ce.ID, ce.name.c_str());
		DoCommandList(ce.event_commands);
		PopStatus();
	}
	if (!gOpt.dryRun) {
		lcf::LDB_Reader::Save(gTranslateDir + "/RPG_RT.ldb", *db, gEncoding);
	}
	PopStatus();
}

void DoLmu(lcf::rpg::Map* lmu) {
	DOASSET(PANORAMA, lmu->parallax_name);
	for (auto& event : lmu->events) {
		PushStatus("Event[%04d] %s @ [%03d,%03d]/", event.ID, event.name.c_str(), event.x, event.y);
		for (auto& page : event.pages) {
			PushStatus("PAGE[%02d]/", page.ID);

			DOASSET(CHARSET, page.character_name);

			PushStatus("Move route/");
			DoMoveRoute(page.move_route);
			PopStatus();

			DoCommandList(page.event_commands);

			PopStatus();
		}
		PopStatus();
	}
}

void DoLmt(lcf::rpg::TreeMap* lmt) {
	PushStatus("LMT/");
	for (auto& mapInfo : lmt->maps) {
		if (mapInfo.ID == 0) continue; //id 0 is the game title for some reason
		PushStatus("Map[%04d] %s/", mapInfo.ID, mapInfo.name.c_str());
		DOASSET(BACKDROP, mapInfo.background_name);
		DOSFX(MUSIC, mapInfo.music.name);

		char mapname[13];
		sprintf(mapname, "/Map%04d.lmu", mapInfo.ID);
		auto lmu = lcf::LMU_Reader::Load(gGameDir + mapname, gEncoding);
		if (lmu) { //2kki lmt references deleted maps
			DoLmu(lmu.get());
			if (!gOpt.dryRun) {
				lcf::LMU_Reader::Save(gTranslateDir + mapname, *lmu, gEngineVer, gEncoding);
			}
		}
		PopStatus();
	}
	if (!gOpt.dryRun) {
		lcf::LMT_Reader::Save(gTranslateDir + "/RPG_RT.lmt", *lmt, gEngineVer, gEncoding);
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
		"		EXAMPLE: -m ./assetmap.txt\n"
		"\n"
		"-M		Print the asset map after execution.\n"
		"\n"
		"-r		Load provided asset map in reverse, if any.\n"
		"\n"
		// frankly this is fucking useless
		//"-t		Set timestamps from the translated files to the year 2000.\n"
		//"\n"
		"-D		DO NOT alert about identically named files.\n"
		"\n"
		"-c		Specify the codepage to use when reading game text.\n"
		"		Default is 932 (shift-jis).\n"
		"		EXAMPLE: -c 932\n"
		"\n"
		/*"-C		Specify the codepage to use when reading arguments\n"
		"		given to this program. Default is the same as the\n"
		"		game text codepage (shift-jis). MUST BE BEFORE ANY\n"
		"		OTHER ARGUMENTS.\n"
		"		EXAMPLE: -C 932\n"*/, a
	);
}

int main(int argc, char** argv)
{
#ifdef _WIN32
	SetConsoleOutputCP(CP_UTF8); //set console to utf8 
#endif
	setlocale(LC_ALL, "en_US.UTF-8");
	gEncoding = lcf::ReaderUtil::CodepageToEncoding(932);
	//std::string argEncoding = "";

	opterr = 0;
	char c = 0;
	//pissed off at my inconsistent usage of stl and c functions? GOOD
	while ((c = getopt(argc, argv, "dhum:Mrtc:D")) != -1) { 
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
				std::ifstream f(optarg);
				if (f.bad()) {
					std::cout << "Could not open asset map file" << std::endl; exit(-1);
				}
				FileToAssetMap(f);
				f.close();
			}
			break;
		case 'M':
			gOpt.printAssList = true;
			break;
		case 'R':
			gOpt.loadAssReverse = true;
			break;
		case 't':
			gOpt.removeDates = true;
			break;
		case 'c':
			gEncoding = lcf::ReaderUtil::CodepageToEncoding(atoi(optarg));
			break;
		case 'D':
			gOpt.alertDupeFiles = false;
			break;
		case '?':
			std::cout << "Blah blah blah error" << std::endl;
			abort();
			break;
		default:
			PrintUsage(argv[0]); exit(-1);
			break;
		}
	}

	for (int i = optind; i < argc; i++) {
		gGameDir = argv[i];
	}

	if (gGameDir.empty()) {
		PrintUsage(argv[0]);
		std::cout << "No game directory specified" << std::endl;
		exit(-1);
	}
	else if (!std::filesystem::exists(gGameDir)) {
		PrintUsage(argv[0]);
		std::cout << "Specified game directory does not exist" << std::endl;
		exit(-1);
	}
	
	gTranslateDir = gGameDir + "/" + "TRANSLATED";
	if (!gOpt.dryRun) {
		if (!std::filesystem::exists(gTranslateDir)) {
			std::filesystem::create_directory(gTranslateDir);
			/*for (auto dir : FOLDERLIST) {
				std::string accum;
				std::filesystem::create_directory(accum = (gTranslateDir + "/" + dir));
			}*/
		}
		else {
			std::cout << "Conversion directory already exists; enabling dry run flag" << std::endl;
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

	// Copy everything over now, translated
	if (!gOpt.dryRun) {
		try {std::filesystem::copy(gGameDir + "/RPG_RT.ini", gTranslateDir + "/RPG_RT.ini"); } catch (...) {}
		try {std::filesystem::copy(gGameDir + "/RPG_RT.exe", gTranslateDir + "/RPG_RT.exe"); } catch (...) {}
		try {std::filesystem::copy(gGameDir + "/ultimate_rt_eb.dll", gTranslateDir + "/ultimate_rt_eb.dll"); } catch (...) {}
		for (auto& entry : gAssetMap) {
			auto& tuple = entry.second;
			auto outFile = gTranslateDir + "/" + std::get<0>(tuple) + "/" + std::get<2>(tuple);
			std::replace(outFile.begin(), outFile.end(), '\\', '/'); //again, this path class has an aneurysm with backslashes
			auto outPath = std::filesystem::path(outFile);
			if(!std::filesystem::exists(outPath.parent_path())) { // fuck off ゆめ2っき\Sound\marimba
				std::filesystem::create_directories(outPath.parent_path());
			}
			//yes, i have to handle this because of 2kki directory traversing that reutilises files
			if (!std::filesystem::exists(outPath)) {
				std::filesystem::copy_file(
					gGameDir + "/" + std::get<0>(tuple) + "/" + std::get<1>(tuple), outPath);
			}
		}
		if(gOpt.removeDates)
			NullDirectoryTimes(gTranslateDir);
	}

	return 0;
}
