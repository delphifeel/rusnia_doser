cmake_minimum_required(VERSION 3.14)

project(rusnia_doser)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_COMPILE_FEATURES ON)

set(SOURCES
	main.cpp 
	json.hpp 
	curl_wrapper.hpp 
	response.hpp
	utils.hpp
	attacker.hpp
)
set(BUILD_SHARED_LIBS False)

add_subdirectory(curl)
add_subdirectory(openssl-cmake)

find_package(Threads REQUIRED)

message("${CURL_LIBRARIES}")

add_executable(rusnia_doser ${SOURCES})

target_link_libraries(rusnia_doser
	Threads::Threads
	ssl
	libcurl
)
target_include_directories(rusnia_doser PRIVATE ${CURL_INCLUDE_DIRS})

if(CMAKE_BUILD_TYPE MATCHES Debug)
	set_target_properties(${PROJECT_NAME} 
		PROPERTIES EXPORT_COMPILE_COMMANDS true)

	execute_process(
		COMMAND ${CMAKE_COMMAND} -E create_symlink
			${CMAKE_BINARY_DIR}/compile_commands.json
			${CMAKE_SOURCE_DIR}/compile_commands.json
	)
endif()
