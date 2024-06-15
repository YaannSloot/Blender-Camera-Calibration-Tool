cmake_minimum_required (VERSION 3.16)

# Enable Hot Reload for MSVC compilers if supported.
if(POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("BlenderCalibrationApp" VERSION 1.0.0 LANGUAGES CXX)

find_package(OpenCV REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(protobuf REQUIRED)
find_package(glog REQUIRED)
find_package(Boost 1.85 REQUIRED)
find_package(Qt6 6.2 COMPONENTS Widgets REQUIRED)

qt_standard_project_setup(REQUIRES 6.5)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message(STATUS "Using ${CMAKE_CXX_COMPILER_ID}")

set(ADDITIONAL_COMPILE_OPTIONS)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(
        /bigobj
        /W4
        /arch:AVX2
        /Ob2
        /Oi
    )
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"
     OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
)
    add_compile_options(
        -fPIC
        -save-temps
        -mavx2
        -O2
    )
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(${Boost_INCLUDE_DIR})

file(GLOB EXEC_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.ui
)

qt_add_executable(calibration ${EXEC_SOURCES})

target_link_libraries(calibration PRIVATE
    Qt6::Widgets
    ${OpenCV_LIBS}
)

set_target_properties(calibration PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)