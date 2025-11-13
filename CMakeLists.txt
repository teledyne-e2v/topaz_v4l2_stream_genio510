cmake_minimum_required(VERSION 3.12)
project(sensortest
	VERSION 1.0.0
	DESCRIPTION "Topaz5d test"
	LANGUAGES C CXX)

add_executable(sensortest
	main.cpp
	SensorV4L.cpp
	SensorV4L.h
)

target_compile_definitions(sensortest PUBLIC -D_UNIX_ -D_LINUX_)

target_include_directories(sensortest PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/v4l-utils/lib/include
)
