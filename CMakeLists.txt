CMAKE_MINIMUM_REQUIRED(VERSION 2.8.8)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_SOURCE_DIR}/cmake/modules")

if(CROSS_COMPILE MATCHES "x86_64")
elseif (CROSS_COMPILE MATCHES "x86")
    message(STATUS "Will build 32-bit binaries!")
    set(CMAKE_TOOLCHAIN_FILE "cmake/cross/i686.cmake")
elseif(CROSS_COMPILE)
    # Full-blown cross-compilation with external sysroot
    message(STATUS "We're cross-compiling!")
    set(CMAKE_TOOLCHAIN_FILE "cmake/cross/LinuxCrossCompile.cmake")
endif()

PROJECT(uhid)
SET(PROJECT_VERSION   0.2.1)
SET(UHID_API_VERSION  2)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_GNU_SOURCE=1 -Wall")

enable_testing()

if (WIN32)
  set(CPACK_GENERATOR "ZIP")
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mno-ms-bitfields")
else()
  set(CPACK_GENERATOR "TGZ")
endif()

set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}-${CMAKE_BUILD_TYPE}-${PROJECT_VERSION}")

include(CPack)
include (GenerateExportHeader)
include(CheckCCompilerFlag)
function(add_cflag_if_supported flag)
    string(REPLACE "-" "_" flagclean ${flag})
    string(REPLACE "=" "_" flagclean ${flagclean})
    string(REPLACE "+" "_" flagclean ${flagclean})
    string(REPLACE "," "_" flagclean ${flagclean})
    string(TOUPPER ${flagclean} flagclean)

    check_c_compiler_flag(${flag} C_SUPPORTS${flagclean})

    if (C_SUPPORTS${flagclean})
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${flag}" PARENT_SCOPE)
    endif()
endfunction()


add_cflag_if_supported("-std=gnu99")
if (CMAKE_BUILD_TYPE MATCHES "Debug")
  add_cflag_if_supported("-g")
endif()

if (CMAKE_BUILD_TYPE MATCHES "StaticRelease")
  if (WIN32 OR MINGW OR MSYS)
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc")
  endif()
endif()

find_package(HidApi)

set(SRCS ${SRCS}
    libuhid.c crc32.c manager.c
    ${HIDAPI_SOURCES})
INCLUDE_DIRECTORIES(
    ./include/
    ${HIDAPI_INCLUDE_DIRS}
    ${CMAKE_BINARY_DIR}
)


set(UHID_EXPORT_GLUE ${CMAKE_BINARY_DIR}/uhid_export_glue.h)


ADD_LIBRARY(uhidstatic STATIC ${SRCS})
SET_TARGET_PROPERTIES(uhidstatic PROPERTIES OUTPUT_NAME uhid)
set_target_properties(uhidstatic PROPERTIES
  COMPILE_FLAGS -DUHID_STATIC)

GENERATE_EXPORT_HEADER(uhidstatic
  BASE_NAME UHID
  EXPORT_MACRO_NAME UHID_API
  EXPORT_FILE_NAME ${UHID_EXPORT_GLUE}
  STATIC_DEFINE UHID_STATIC
)

if (NOT CMAKE_BUILD_TYPE MATCHES "StaticRelease")
ADD_LIBRARY(uhidshared SHARED ${SRCS})

TARGET_LINK_LIBRARIES(uhidshared
    ${HIDAPI_LIBRARIES}
)
SET_TARGET_PROPERTIES(uhidshared PROPERTIES OUTPUT_NAME uhid)
SET_TARGET_PROPERTIES(uhidshared PROPERTIES SOVERSION ${PROJECT_VERSION}
  VERSION ${UHID_API_VERSION})

INSTALL(TARGETS uhidshared
  RUNTIME DESTINATION lib/${CMAKE_LIBRARY_PATH}
  LIBRARY DESTINATION lib/${CMAKE_LIBRARY_PATH}
  ARCHIVE DESTINATION lib/${CMAKE_LIBRARY_PATH}
  COMPONENT lib
)

endif()

ADD_EXECUTABLE(uhidtool uhidtool.c)

if (CMAKE_BUILD_TYPE MATCHES "StaticRelease")
  set_target_properties(uhidtool PROPERTIES
    COMPILE_FLAGS -DUHID_STATIC)
  TARGET_LINK_LIBRARIES(uhidtool uhidstatic ${HIDAPI_STATIC_LIBRARIES})
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

INSTALL(TARGETS uhidstatic ARCHIVE
        DESTINATION lib/${CMAKE_LIBRARY_PATH})
INSTALL(TARGETS uhidtool RUNTIME
  DESTINATION bin)

file(GLOB UHID_HEADERS
    "${CMAKE_SOURCE_DIR}/include/*"
)

INSTALL(FILES ${UHID_HEADERS} ${UHID_EXPORT_GLUE}
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
