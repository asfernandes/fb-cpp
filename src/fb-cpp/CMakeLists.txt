set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(fb-cpp CXX)

set(_fb_cpp_use_boost_dll_default ON)
if(DEFINED VCPKG_INSTALLED_DIR AND DEFINED VCPKG_TARGET_TRIPLET)
	if(NOT EXISTS "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/boost-dll")
		set(_fb_cpp_use_boost_dll_default OFF)
	endif()
endif()
option(FB_CPP_USE_BOOST_DLL "Enable Boost.DLL support for loading fbclient at runtime"
	${_fb_cpp_use_boost_dll_default})
unset(_fb_cpp_use_boost_dll_default)

set(_fb_cpp_use_boost_multiprecision_default ON)
if(DEFINED VCPKG_INSTALLED_DIR AND DEFINED VCPKG_TARGET_TRIPLET)
	if(NOT EXISTS "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/boost-multiprecision")
		set(_fb_cpp_use_boost_multiprecision_default OFF)
	endif()
endif()
option(FB_CPP_USE_BOOST_MULTIPRECISION "Enable Boost.Multiprecision helpers for INT128 and DECFLOAT types"
	${_fb_cpp_use_boost_multiprecision_default})
unset(_fb_cpp_use_boost_multiprecision_default)

set(_fb_cpp_use_boost_decimal_default ON)
if(DEFINED VCPKG_INSTALLED_DIR AND DEFINED VCPKG_TARGET_TRIPLET)
	if(NOT EXISTS "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/boost-decimal")
		set(_fb_cpp_use_boost_decimal_default OFF)
	endif()
endif()
option(FB_CPP_USE_BOOST_DECIMAL "Enable Boost.Decimal helpers for DECFLOAT types"
	${_fb_cpp_use_boost_decimal_default})
unset(_fb_cpp_use_boost_decimal_default)

file(GLOB_RECURSE SRC
	"*.h"
	"*.cpp"
)

set(_fb_cpp_boost_components "")
if(FB_CPP_USE_BOOST_DLL)
	list(APPEND _fb_cpp_boost_components dll)
endif()
if(FB_CPP_USE_BOOST_MULTIPRECISION)
	list(APPEND _fb_cpp_boost_components multiprecision)
endif()
if(FB_CPP_USE_BOOST_DECIMAL)
	list(APPEND _fb_cpp_boost_components decimal)
endif()

if(_fb_cpp_boost_components)
	find_package(Boost REQUIRED
		COMPONENTS ${_fb_cpp_boost_components}
	)
endif()

find_package(firebird CONFIG REQUIRED)


add_library(${PROJECT_NAME}
	${SRC}
)

if(FB_CPP_USE_BOOST_DECIMAL)
	target_compile_options(${PROJECT_NAME}
		PUBLIC
			$<$<CXX_COMPILER_ID:GNU>:-Wno-psabi>
	)
endif()

include(GenerateExportHeader)
generate_export_header(${PROJECT_NAME}
	BASE_NAME FB_CPP
	EXPORT_FILE_NAME "${CMAKE_CURRENT_BINARY_DIR}/fb-cpp-export.generated.h"
)

target_include_directories(${PROJECT_NAME}
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/..>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
		$<INSTALL_INTERFACE:include>
)

if(NOT DEFINED FB_CPP_USE_BOOST_DLL_VALUE)
	if(FB_CPP_USE_BOOST_DLL)
		set(FB_CPP_USE_BOOST_DLL_VALUE 1)
	else()
		set(FB_CPP_USE_BOOST_DLL_VALUE 0)
	endif()
endif()

if(NOT DEFINED FB_CPP_USE_BOOST_MULTIPRECISION_VALUE)
	if(FB_CPP_USE_BOOST_MULTIPRECISION)
		set(FB_CPP_USE_BOOST_MULTIPRECISION_VALUE 1)
	else()
		set(FB_CPP_USE_BOOST_MULTIPRECISION_VALUE 0)
	endif()
endif()

if(NOT DEFINED FB_CPP_USE_BOOST_DECIMAL_VALUE)
	if(FB_CPP_USE_BOOST_DECIMAL)
		set(FB_CPP_USE_BOOST_DECIMAL_VALUE 1)
	else()
		set(FB_CPP_USE_BOOST_DECIMAL_VALUE 0)
	endif()
endif()

target_compile_definitions(${PROJECT_NAME}
	PUBLIC
		FB_CPP_USE_BOOST_DLL=${FB_CPP_USE_BOOST_DLL_VALUE}
		FB_CPP_USE_BOOST_MULTIPRECISION=${FB_CPP_USE_BOOST_MULTIPRECISION_VALUE}
		FB_CPP_USE_BOOST_DECIMAL=${FB_CPP_USE_BOOST_DECIMAL_VALUE}
)

target_link_libraries(${PROJECT_NAME}
	PUBLIC
		firebird
)

if(FB_CPP_USE_BOOST_DLL)
	target_link_libraries(${PROJECT_NAME}
		PUBLIC Boost::dll
	)
endif()

if(FB_CPP_USE_BOOST_MULTIPRECISION)
	target_link_libraries(${PROJECT_NAME}
		PUBLIC Boost::multiprecision
	)
endif()

if(FB_CPP_USE_BOOST_DECIMAL)
	target_link_libraries(${PROJECT_NAME}
		PUBLIC Boost::decimal
	)
endif()

unset(_fb_cpp_boost_components)

file(GLOB_RECURSE HEADER_FILES
	"${CMAKE_CURRENT_LIST_DIR}/*.h"
	"${CMAKE_CURRENT_LIST_DIR}/*.hpp"
)

foreach(HEADER_FILE ${HEADER_FILES})
	file(RELATIVE_PATH REL_PATH ${CMAKE_CURRENT_LIST_DIR} ${HEADER_FILE})
	get_filename_component(REL_DIR ${REL_PATH} DIRECTORY)
	if(REL_DIR)
		install(FILES ${HEADER_FILE}
			DESTINATION include/fb-cpp/${REL_DIR}
		)
	else()
		install(FILES ${HEADER_FILE}
			DESTINATION include/fb-cpp
		)
	endif()
endforeach()

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/fb-cpp-export.generated.h"
	DESTINATION include/fb-cpp
)

install(TARGETS ${PROJECT_NAME}
	EXPORT ${PROJECT_NAME}Targets
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	RUNTIME DESTINATION bin
)

install(EXPORT ${PROJECT_NAME}Targets
	FILE ${PROJECT_NAME}Targets.cmake
	NAMESPACE ${PROJECT_NAME}::
	DESTINATION fb-cpp/cmake/${PROJECT_NAME}
)
