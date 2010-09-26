project(uloz-to-daemon)
cmake_minimum_required(VERSION 2.6.0 FATAL_ERROR)

find_package(Qt4 REQUIRED)

include_directories(src)

set(CMAKE_BUILD_TYPE Debug)

set(QT_USE_QTCORE TRUE)
set(QT_USE_QTNETWORK TRUE)

include(${QT_USE_FILE})
add_definitions(${QT_DEFINITIONS})
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${QT_INCLUDE_DIR})

set(SRCS
	src/main.cpp
	src/CCaptcha.cpp
	src/CCaptchaLetter.cpp
	src/CDaemon.cpp
	src/CDaemon_communication.cpp
	src/CDownload.cpp
	src/CDownloadManager.cpp)

set(MOC_HDRS
	src/CCaptcha.h
	src/CDaemon.h
	src/CDownload.h
	src/CDownloadManager.h)

qt4_wrap_cpp(MOC_SRCS ${MOC_HDRS})

add_executable(uloz-to-daemon ${SRCS} ${MOC_SRCS})
target_link_libraries(uloz-to-daemon ${QT_LIBRARIES} /usr/lib64/libmad.so)
