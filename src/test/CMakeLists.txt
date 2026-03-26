set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(Boost_NO_WARN_NEW_VERSIONS 1)

project(fb-cpp-test CXX)

include(BoostTestDiscoverTests)

file(GLOB_RECURSE SRC
	"*.h"
	"*.cpp"
)


add_executable(${PROJECT_NAME}
	${SRC}
)

find_package(Boost REQUIRED
	COMPONENTS
		filesystem
		unit_test_framework
)

find_package(firebird REQUIRED)

target_include_directories(${PROJECT_NAME}
	PUBLIC ${Boost_INCLUDE_DIR}
)

target_link_libraries(${PROJECT_NAME}
	PRIVATE fb-cpp
	PRIVATE firebird
	PRIVATE Boost::filesystem
	PRIVATE Boost::unit_test_framework
)

boost_test_discover_tests(${PROJECT_NAME}
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	DISCOVERY_MODE PRE_TEST
	PROPERTIES
		ENVIRONMENT "FB_CPP_BUILD_DIR=${CMAKE_BINARY_DIR};FB_CPP_BUILD_CONFIG=$<CONFIG>"
)
