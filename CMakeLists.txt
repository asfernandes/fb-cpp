cmake_minimum_required(VERSION 3.18)

set(VCPKG_ROOT "vcpkg")
set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

set(CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR} ${CMAKE_MODULE_PATH})
set(CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR} ${CMAKE_PREFIX_PATH})

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out/bin)

project(fb-cpp-main
	VERSION 0.0.1
	DESCRIPTION "C++ client library for Firebird"
	HOMEPAGE_URL "https://github.com/asfernandes/fb-cpp"
	LANGUAGES CXX
)

enable_language(CXX)


if(MSVC)
	add_compile_options(
		/W4
		/permissive-
		/w44062			# enumerator 'identifier' in switch of enum 'enumeration' is not handled
		/w44265			# 'class': class has virtual functions, but destructor is not virtual
		# disable
		/wd4100			# 'identifier': unreferenced parameter
	)
else()
	add_compile_options(
		-Wall
		-Wextra
		-Wpedantic
		-Wconversion
		-Wno-unused-parameter
	)
endif()

add_subdirectory(src/lib)
add_subdirectory(src/test)
