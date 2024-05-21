# CMakeList.txt : CMake project for RmkDelocalise, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (RmkDelocalise "RmkDelocalise.cpp" "RmkDelocalise.h" "md5.c" "md5.h" "getopt.h")


set_property(TARGET RmkDelocalise PROPERTY CXX_STANDARD 17)
set(TARGET RmkDelocalise PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>$<$<STREQUAL:${VCPKG_CRT_LINKAGE},dynamic>:DLL>")

target_link_libraries(RmkDelocalise lcf)

# TODO: Add tests and install targets if needed.
