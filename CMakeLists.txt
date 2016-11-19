CMAKE_MINIMUM_REQUIRED(VERSION 2.8.8)

PROJECT(uhid)
SET(PROJECT_VERSION   0.2.1)
SET(UHID_API_VERSION  2)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -g -D_GNU_SOURCE=1")

enable_testing()
set(CPACK_GENERATOR "ZIP")
include(CPack)

if (WIN32)
  find_path(HIDAPI_DIR hidapi/hidapi.h ${CMAKE_SOURCE_DIR}/deps/hidapi ${CMAKE_SOURCE_DIR}/deps/hidapi-0.7.0 ${UHID_HIDAPI_DIR})
  if (HIDAPI_DIR-NOTFOUND)
    message(ERROR "Please download and unpack to hidapi to ${CMAKE_SOURCE_DIR}/deps/hidapi")
  else()
    message(STATUS "Using hidapi in ${HIDAPI_DIR}")
  endif()

  set(HIDAPI_INCLUDE_DIRS "${HIDAPI_DIR};${HIDAPI_DIR}/hidapi")
  set(SRCS "${HIDAPI_DIR}/windows/hid.c")
  set(HIDAPI_LIBRARIES "setupapi")
else()
  FIND_PACKAGE(PkgConfig)
  PKG_CHECK_MODULES(HIDAPI hidapi-libusb)
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
endif()

set(SRCS ${SRCS} libuhid.c)
message("${SRCS}")
INCLUDE_DIRECTORIES(
    ./include/
    ${HIDAPI_INCLUDE_DIRS}
)


ADD_LIBRARY(uhid OBJECT ${SRCS})
ADD_LIBRARY(uhidstatic STATIC $<TARGET_OBJECTS:uhid>)
SET_TARGET_PROPERTIES(uhidstatic PROPERTIES OUTPUT_NAME uhid)
TARGET_LINK_LIBRARIES(uhidstatic
    ${HIDAPI_LIBRARIES}
)

ADD_LIBRARY(uhidshared SHARED $<TARGET_OBJECTS:uhid>)
SET_TARGET_PROPERTIES(uhidshared PROPERTIES OUTPUT_NAME uhid)
SET_TARGET_PROPERTIES(uhidshared PROPERTIES SOVERSION ${PROJECT_VERSION}
  VERSION ${UHID_API_VERSION})

TARGET_LINK_LIBRARIES(uhidshared
    ${HIDAPI_LIBRARIES}
)

ADD_EXECUTABLE(uhidtool uhidtool.c)

#Static link for windows
if(WIN32)
  TARGET_LINK_LIBRARIES(uhidtool uhidstatic)
else()
  TARGET_LINK_LIBRARIES(uhidtool uhidshared)
endif()

if (ENABLE_TESTS_AVR)
  ADD_TEST(test-flash ${CMAKE_SOURCE_DIR}/tests/write-verify.sh
    ${CMAKE_BINARY_DIR}/uhidtool flash 6
    )

  ADD_TEST(test-eeprom ${CMAKE_SOURCE_DIR}/tests/write-verify.sh
    ${CMAKE_BINARY_DIR}/uhidtool eeprom 6
    )
endif()

INSTALL(TARGETS uhidshared
  LIBRARY DESTINATION lib/${CMAKE_LIBRARY_PATH}
  ARCHIVE DESTINATION lib/${CMAKE_LIBRARY_PATH}
  COMPONENT lib
)

INSTALL(TARGETS uhidstatic ARCHIVE
        DESTINATION lib/${CMAKE_LIBRARY_PATH})
INSTALL(TARGETS uhidtool RUNTIME
  DESTINATION bin)

file(GLOB UHID_HEADERS
    "${CMAKE_SOURCE_DIR}/include/*"
)

INSTALL(FILES ${UHID_HEADERS}
DESTINATION include/${CMAKE_LIBRARY_PATH}/uhid-${PROJECT_VERSION}/)


macro(generate_clang_complete)
  message(STATUS "Generarating ${CMAKE_SOURCE_DIR}/.clang_complete")
  get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
  file(WRITE ${CMAKE_SOURCE_DIR}/.clang_complete "")
  foreach(dir ${dirs})
    file(APPEND ${CMAKE_SOURCE_DIR}/.clang_complete "-I${dir}\n")
  endforeach()
  string(REPLACE "'" "" CMAKE_C_FLAGS_SPLIT ${CMAKE_C_FLAGS})
  string(REPLACE " " ";" CMAKE_C_FLAGS_SPLIT ${CMAKE_C_FLAGS_SPLIT})
  foreach(flag ${CMAKE_C_FLAGS_SPLIT})
  # FixMe: Ugly hack
    if (NOT "${flag}" MATCHES "-Wall")
      if (NOT "${flag}" MATCHES "-g")
        if (NOT "${flag}" MATCHES "-fPIC")
          file(APPEND ${CMAKE_SOURCE_DIR}/.clang_complete "${flag}\n")
        endif()
      endif()
    endif()
  endforeach()
endmacro()

if(NOT WIN32)
  generate_clang_complete()
endif()
