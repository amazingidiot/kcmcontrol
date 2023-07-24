cmake_minimum_required(VERSION 3.14)

set(PROGRAM_NAME kcmcontrol)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(${PROGRAM_NAME} LANGUAGES CXX)

FIND_PACKAGE (
  Qt6
  COMPONENTS
    Core
    Network
  REQUIRED )

set(SOURCE_FILES
    src/main.cpp
    src/client_listener.cpp
    src/client_manager.cpp
    src/client_watcher.cpp
    src/jack_client.cpp
    src/jack_input.cpp
    src/jack_output.cpp
    src/osc_client.cpp
    src/osc_endpoint.cpp
    src/osc_message.cpp
    src/osc_processor.cpp
    src/sigwatch.cpp
)

set(HEADER_FILES
    include/client_listener.h
    include/client_manager.h
    include/client_watcher.h
    include/jack_client.h
    include/jack_input.h
    include/jack_output.h
    include/osc_client.h
    include/osc_endpoint.h
    include/osc_message.h
    include/osc_processor.h
    include/sigwatch.h
)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

add_executable(${PROGRAM_NAME}
  ${SOURCE_FILES}
  ${HEADER_FILES}
)

install(TARGETS ${PROGRAM_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

TARGET_LINK_LIBRARIES ( ${PROGRAM_NAME}
  Qt6::Core
  Qt6::Network
  jack
)
