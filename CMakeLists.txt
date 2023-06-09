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
    src/osc_client.cpp
    src/osc_message.cpp
    src/osc_server.cpp
    src/sigwatch.cpp
)

set(HEADER_FILES
    include/osc_client.h
    include/osc_message.h
    include/osc_server.h
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
)
