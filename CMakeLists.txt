# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

set(CMAKE_CXX_STANDARD 17)

if(WIN32)
	# Target Unicode API
	add_definitions(-D_UNICODE)
	add_definitions(-DUNICODE)

	# Disable API deprecation warnings
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)

	# Prevent some Windows.h global namespace pollution
	add_definitions(-DNOMINMAX)
	add_definitions(-DWIN32_LEAN_AND_MEAN)
	add_compile_options("/utf-8")
endif()

project ("RmkDelocalise")
set(LIBLCF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/liblcf")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/liblcf/builds/cmake/Modules")

function(add_liblcf)
		set(BUILD_SHARED_LIBS OFF)
		set(LIBLCF_ENABLE_INSTALL OFF)
		set(SHARED_RUNTIME OFF)
		add_subdirectory(${LIBLCF_PATH})
endfunction()
add_liblcf()

# Include sub-projects.
add_subdirectory ("RmkDelocalise")
