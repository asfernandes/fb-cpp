set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(Boost_NO_WARN_NEW_VERSIONS 1)

project(fb-cpp-test CXX)

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

add_test(
	NAME ${PROJECT_NAME}
	COMMAND ${PROJECT_NAME}
)
