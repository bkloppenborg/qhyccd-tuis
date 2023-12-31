cmake_minimum_required(VERSION 3.8.2)

find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(OpenCV REQUIRED)
find_package(CFITSIO REQUIRED)

add_library(cvfits cvfits.cpp coordinate_conversions.cpp)

target_link_libraries(cvfits Qt6::Core ${OpenCV_LIBS} CFITSIO::CFITSIO )

target_include_directories(cvfits
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
