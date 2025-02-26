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
#include <lcf/rpg/moveroute.h>
#include <lcf/rpg/movecommand.h>

#include <string>
#include <iterator>
#include <vector>
#include <cstdlib>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <unistd.h>

std::string gEncoding = "";// = "ibm-943_P15A-2003";

int DecodeInt(std::vector<int>::const_iterator& it) {
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
std::string DecodeString(std::vector<int>::const_iterator& it) {
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
	return result;
}

lcf::rpg::MoveCommand DecodeMove(std::vector<int>::const_iterator& it) {
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

lcf::rpg::MoveRoute CommandParamsToMoveRoute(std::vector<int>& params) {
	lcf::rpg::MoveRoute moveRoute;
	moveRoute.repeat = params[2] != 0;
	moveRoute.skippable = params[3] != 0;
	for (std::vector<int>::const_iterator it = params.begin() + 4; it < params.end();) {
		moveRoute.move_commands.push_back(DecodeMove(it));
	}
	return moveRoute;
}



int main(int argc, char** argv) {
	#ifdef _WIN32
	SetConsoleOutputCP(CP_UTF8); //set console to utf8 
	#endif
	setlocale(LC_ALL, "en_US.UTF-8");
	gEncoding = lcf::ReaderUtil::CodepageToEncoding(932);
	
	opterr = 0;
	char c = 0;
	while ((c = getopt(argc, argv, "m:")) != -1) { 
		switch (c) {
		case 'm':
			gEncoding = lcf::ReaderUtil::CodepageToEncoding(atoi(optarg));
			break;
		default:
			puts("what");
			return -1;
		}
	}

	std::vector<int> cmds;
	for (int i = optind; i < argc; i++) {
		cmds.push_back(atoi(argv[i]));
	}
	if(cmds.size() == 0)
		return -1;
	auto route = CommandParamsToMoveRoute(cmds);
	printf("Repeats: %s, Skippable: %s\n", route.repeat ? "Yes" : "No", route.skippable ? "Yes" : "No");
	for(auto cmd : route.move_commands) {
		auto name = lcf::rpg::MoveCommand::kCodeTags.tag(cmd.command_id);
		switch (cmd.command_id) {
			case 32:	// Switch ON
			case 33:	// Switch OFF
				printf("%s %04d\n", name, cmd.parameter_a);
				break;
			case 34:	// Change Graphic
				printf("%s %s [%d]\n", name, cmd.parameter_string.c_str(), cmd.parameter_a);
				break;
			case 35:	// Play Sound Effect
				printf("%s %s [VOL: %d TEMPO: %d PAN: %d]\n", name, cmd.parameter_string.c_str(), cmd.parameter_a, cmd.parameter_b, cmd.parameter_c);
				break;
			default:
				puts(name);
				break;
		}
	}
	return 0;
}