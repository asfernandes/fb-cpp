cmake_minimum_required(VERSION 3.18)

find_package(Doxygen)

if(DOXYGEN_FOUND)
	configure_file(
		${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile
		${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
		@ONLY
	)

	file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/doxygen-awesome-css)

	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/doxygen-awesome-css/
		DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/doxygen-awesome-css/
		FILES_MATCHING PATTERN "*.css" PATTERN "*.js"
	)

	add_custom_target(docs
		COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		COMMENT "Generating API documentation with Doxygen"
		VERBATIM
	)

	add_dependencies(docs fb-cpp)

	install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/docs/
		DESTINATION share/doc/fb-cpp
		OPTIONAL
	)
else()
	message(STATUS "Doxygen not found - documentation target will not be available")
endif()
