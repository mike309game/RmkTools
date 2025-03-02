#include "RmkDelocalise.hh"

static void NullFileTime(std::string_view file) {
#ifdef _WIN32
	int wstrlen = MultiByteToWideChar(CP_UTF8, 0, file.data(), file.length()+1, NULL, 0);
	wchar_t* wstr = new wchar_t[wstrlen];
	MultiByteToWideChar(CP_UTF8, 0, file.data(), file.length()+1, wstr, wstrlen);
	//std::replace(wstr, wstr+wstrlen, '/', '\\');

	// abuse FILE_FLAG_BACKUP_SEMANTICS????????????
	// without it it won't open folders
	HANDLE hFile = CreateFileW(wstr, GENERIC_READ | GENERIC_WRITE | DELETE, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		std::wcout << GetLastError() << " Could not open file to set time?!?!?!?! " << wstr << std::endl; exit(-1);
	}
	FILE_BASIC_INFO b = {};
	//leaving at 0 or -1: just sets time to current one
	//leaving at 1: rpg_rt can't find files??, crashes std::filesystem functions?????????????????
	//leaving at INT64_MAX: rpg_rt can't find files??
	b.LastWriteTime.QuadPart = 125911584000000000ll;
	b.ChangeTime.QuadPart = 125911584000000000ll;
	b.CreationTime.QuadPart = 125911584000000000ll;
	b.LastAccessTime.QuadPart = 125911584000000000ll;
	BY_HANDLE_FILE_INFORMATION bb;
	GetFileInformationByHandle(hFile, &bb);
	b.FileAttributes = bb.dwFileAttributes;
	SetFileInformationByHandle(hFile, FileBasicInfo, &b, sizeof(b));
	CloseHandle(hFile);
	delete[] wstr;
#else
	struct utimbuf t;
	t.actime = 0;
	t.modtime = 0;
	utime(file.c_str(), &t);
#endif
}

void NullDirectoryTimes(std::string_view dir) {
	for(const auto& entry : std::filesystem::directory_iterator(dir)) {
		if(entry.is_directory())
			NullDirectoryTimes(entry.path().string());
		else
			NullFileTime(entry.path().string());
	}
	NullFileTime(dir);
}