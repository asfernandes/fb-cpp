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
	VERSION 0.0.2
	DESCRIPTION "C++ client library for Firebird"
	HOMEPAGE_URL "https://github.com/asfernandes/fb-cpp"
	LANGUAGES CXX
)

enable_language(CXX)
include(CTest)


if(MSVC)
	add_compile_options(
		/W4
		/permissive-
		/w44062			# enumerator 'identifier' in switch of enum 'enumeration' is not handled
		/w44265			# 'class': class has virtual functions, but destructor is not virtual
		# disable
		/wd4100			# 'identifier': unreferenced parameter
	)
	add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
	add_compile_options(
		-Wall
		-Wextra
		-Wpedantic
		-Wconversion
		-Wno-unused-parameter
	)
endif()

add_subdirectory(src/fb-cpp)
add_subdirectory(doc)

if(BUILD_TESTING)
	add_subdirectory(src/test)
endif()


set(FB_CPP_USE_BOOST_DLL_VALUE 0)
if(FB_CPP_USE_BOOST_DLL)
	set(FB_CPP_USE_BOOST_DLL_VALUE 1)
endif()

set(FB_CPP_USE_BOOST_MULTIPRECISION_VALUE 0)
if(FB_CPP_USE_BOOST_MULTIPRECISION)
	set(FB_CPP_USE_BOOST_MULTIPRECISION_VALUE 1)
endif()


include(CMakePackageConfigHelpers)
write_basic_package_version_file(
	"${CMAKE_CURRENT_BINARY_DIR}/fb-cppConfigVersion.cmake"
	VERSION ${PROJECT_VERSION}
	COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/fb-cppConfig.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/fb-cppConfig.cmake"
	INSTALL_DESTINATION fb-cpp/cmake/fb-cpp
)

install(FILES
	"${CMAKE_CURRENT_BINARY_DIR}/fb-cppConfig.cmake"
	"${CMAKE_CURRENT_BINARY_DIR}/fb-cppConfigVersion.cmake"
	DESTINATION fb-cpp/cmake/fb-cpp
)
