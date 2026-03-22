cmake_minimum_required(VERSION 3.21)

project(real-time-vector-demo VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

option(DEMO_BUILD_TESTS "Build DEMO tests" ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Network)

find_package(OpenCV QUIET COMPONENTS core imgproc imgcodecs videoio)
if(OpenCV_FOUND)
    message(STATUS "OpenCV found: ${OpenCV_VERSION}")
else()
    message(WARNING "OpenCV was not found. DEMO will build in receiver-only/stub mode until OpenCV is installed.")
endif()

set(DEMO_CORE_SOURCES
    src/core/config/app_config.cpp
    src/core/model/scene_types.cpp
    src/core/network/packet_codec.cpp
    src/core/network/udp_device_receiver.cpp
    src/core/network/udp_device_sender.cpp
    src/core/processing/media_processor.cpp
    src/core/processing/sender_engine.cpp
    src/core/routing/frame_router.cpp
)

set(DEMO_CORE_HEADERS
    src/core/config/app_config.h
    src/core/model/geometry.h
    src/core/model/scene_types.h
    src/core/network/packet_codec.h
    src/core/network/udp_device_receiver.h
    src/core/network/udp_device_sender.h
    src/core/processing/media_processor.h
    src/core/processing/sender_engine.h
    src/core/routing/frame_router.h
)

add_library(demo_core STATIC ${DEMO_CORE_SOURCES} ${DEMO_CORE_HEADERS})
target_include_directories(demo_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(demo_core
    PUBLIC
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Network
)
target_compile_definitions(demo_core
    PUBLIC
        $<$<BOOL:${OpenCV_FOUND}>:DEMO_HAS_OPENCV=1>
        $<$<NOT:$<BOOL:${OpenCV_FOUND}>>:DEMO_HAS_OPENCV=0>
)

if(OpenCV_FOUND)
    target_link_libraries(demo_core PUBLIC ${OpenCV_LIBS})
endif()

add_executable(demo_gui
    src/app/gui/main.cpp
    src/app/gui/main_window.cpp
    src/app/gui/main_window.h
    src/app/gui/preview_widget.cpp
    src/app/gui/preview_widget.h
)
target_link_libraries(demo_gui
    PRIVATE
        demo_core
        Qt${QT_VERSION_MAJOR}::Widgets
)

add_executable(demo_cli
    src/app/cli/main.cpp
)
target_link_libraries(demo_cli PRIVATE demo_core)

if(DEMO_BUILD_TESTS)
    enable_testing()
    add_executable(packet_codec_test tests/packet_codec_test.cpp)
    target_link_libraries(packet_codec_test PRIVATE demo_core)
    add_test(NAME packet_codec_test COMMAND packet_codec_test)
endif()
