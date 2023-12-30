CMAKE_MINIMUM_REQUIRED(VERSION 3.22)

# include the current directory for older versions of CMake
if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

# enable automoc for Qt applications. Delete otherwise.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# include Qt5, if needed
find_package(Qt5 COMPONENTS Widgets REQUIRED)
find_package(QHYCCD REQUIRED)
find_package(OpenCV REQUIRED)

# Include the primary project include directory
include_directories(${PROJECT_SOURCE_DIR}/include)

# List camera application
add_executable(qhy-list-cameras list_cameras.cpp)
target_link_libraries(qhy-list-cameras QHYCCD::QHYCCD)

# Command line interface and test application
add_library(cli-parser cli_parser.cpp)
target_link_libraries(cli-parser Qt5::Core)
add_executable(cli-test cli_test.cpp)
target_link_libraries(cli-test Qt5::Core cli-parser)

# Camera control application
add_executable(qhy-camera-control camera_control.cpp)
target_link_libraries(qhy-camera-control QHYCCD::QHYCCD Qt5::Core cli-parser ${OpenCV_LIBS})
