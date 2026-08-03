function(run_command)
	execute_process(
		COMMAND ${ARGN}
		RESULT_VARIABLE result
		OUTPUT_VARIABLE output
		ERROR_VARIABLE error
	)

	if(NOT result EQUAL 0)
		message(FATAL_ERROR
			"Command failed with exit code ${result}:\n"
			"${output}\n${error}"
		)
	endif()
endfunction()

file(REMOVE_RECURSE
	"${FB_CPP_CONSUMER_BINARY_DIR}"
	"${FB_CPP_INSTALL_PREFIX}"
)

run_command(
	"${CMAKE_COMMAND}"
	--install "${FB_CPP_BUILD_DIR}"
	--prefix "${FB_CPP_INSTALL_PREFIX}"
	--config "${FB_CPP_BUILD_CONFIG}"
)

set(prefixes "${FB_CPP_INSTALL_PREFIX}")
if(FB_CPP_VCPKG_INSTALLED_DIR AND FB_CPP_VCPKG_TARGET_TRIPLET)
	list(APPEND prefixes
		"${FB_CPP_VCPKG_INSTALLED_DIR}/${FB_CPP_VCPKG_TARGET_TRIPLET}"
	)
endif()
list(JOIN prefixes ";" prefix_path)

set(configure_command
	"${CMAKE_COMMAND}"
	-S "${FB_CPP_SOURCE_DIR}/tests/consumer"
	-B "${FB_CPP_CONSUMER_BINARY_DIR}"
	"-DCMAKE_PREFIX_PATH=${prefix_path}"
	"-DCMAKE_BUILD_TYPE=${FB_CPP_BUILD_CONFIG}"
)

if(FB_CPP_CMAKE_TOOLCHAIN_FILE)
	list(APPEND configure_command "-DCMAKE_TOOLCHAIN_FILE=${FB_CPP_CMAKE_TOOLCHAIN_FILE}")
endif()

if(FB_CPP_VCPKG_INSTALLED_DIR)
	list(APPEND configure_command "-DVCPKG_INSTALLED_DIR=${FB_CPP_VCPKG_INSTALLED_DIR}")
endif()

if(FB_CPP_VCPKG_TARGET_TRIPLET)
	list(APPEND configure_command "-DVCPKG_TARGET_TRIPLET=${FB_CPP_VCPKG_TARGET_TRIPLET}")
endif()

run_command(${configure_command})
run_command(
	"${CMAKE_COMMAND}"
	--build "${FB_CPP_CONSUMER_BINARY_DIR}"
	--config "${FB_CPP_BUILD_CONFIG}"
)

if(WIN32)
	set(consumer_executable "${FB_CPP_CONSUMER_BINARY_DIR}/${FB_CPP_BUILD_CONFIG}/fb-cpp-installed-consumer.exe")
	set(runtime_path
		"${FB_CPP_INSTALL_PREFIX}/bin;${FB_CPP_INSTALL_PREFIX}/lib"
	)
	if(FB_CPP_VCPKG_INSTALLED_DIR AND FB_CPP_VCPKG_TARGET_TRIPLET)
		string(APPEND runtime_path
			";${FB_CPP_VCPKG_INSTALLED_DIR}/${FB_CPP_VCPKG_TARGET_TRIPLET}/bin"
			";${FB_CPP_VCPKG_INSTALLED_DIR}/${FB_CPP_VCPKG_TARGET_TRIPLET}/lib"
			";${FB_CPP_VCPKG_INSTALLED_DIR}/${FB_CPP_VCPKG_TARGET_TRIPLET}/debug/bin"
			";${FB_CPP_VCPKG_INSTALLED_DIR}/${FB_CPP_VCPKG_TARGET_TRIPLET}/debug/lib"
		)
	endif()
	set(ENV{PATH} "${runtime_path};$ENV{PATH}")
elseif(APPLE)
	set(consumer_executable "${FB_CPP_CONSUMER_BINARY_DIR}/fb-cpp-installed-consumer")
	set(runtime_path "${FB_CPP_INSTALL_PREFIX}/lib")
	if(FB_CPP_VCPKG_INSTALLED_DIR AND FB_CPP_VCPKG_TARGET_TRIPLET)
		string(APPEND runtime_path
			":${FB_CPP_VCPKG_INSTALLED_DIR}/${FB_CPP_VCPKG_TARGET_TRIPLET}/lib"
			":${FB_CPP_VCPKG_INSTALLED_DIR}/${FB_CPP_VCPKG_TARGET_TRIPLET}/debug/lib"
		)
	endif()
	set(ENV{DYLD_LIBRARY_PATH} "${runtime_path}:$ENV{DYLD_LIBRARY_PATH}")
else()
	set(consumer_executable "${FB_CPP_CONSUMER_BINARY_DIR}/fb-cpp-installed-consumer")
	set(runtime_path "${FB_CPP_INSTALL_PREFIX}/lib")
	if(FB_CPP_VCPKG_INSTALLED_DIR AND FB_CPP_VCPKG_TARGET_TRIPLET)
		string(APPEND runtime_path
			":${FB_CPP_VCPKG_INSTALLED_DIR}/${FB_CPP_VCPKG_TARGET_TRIPLET}/lib"
			":${FB_CPP_VCPKG_INSTALLED_DIR}/${FB_CPP_VCPKG_TARGET_TRIPLET}/debug/lib"
		)
	endif()
	set(ENV{LD_LIBRARY_PATH} "${runtime_path}:$ENV{LD_LIBRARY_PATH}")
endif()

run_command("${consumer_executable}")
