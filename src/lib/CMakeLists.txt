set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(fb-cpp CXX)

file(GLOB_RECURSE SRC
	"*.h"
	"*.cpp"
)


add_library(${PROJECT_NAME}
	${SRC}
)

target_include_directories(${PROJECT_NAME}
	PUBLIC ${CMAKE_CURRENT_LIST_DIR}/
)

target_link_libraries(${PROJECT_NAME}
	${CMAKE_DL_LIBS}
)
