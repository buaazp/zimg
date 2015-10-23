cmake_minimum_required(VERSION 2.8)
project(zimg)

set (PROJECT_MAJOR_VERSION 3)
set (PROJECT_MINOR_VERSION 2)
set (PROJECT_PATCH_VERSION 0)

set (PROJECT_VERSION ${PROJECT_MAJOR_VERSION}.${PROJECT_MINOR_VERSION}.${PROJECT_PATCH_VERSION})
set (CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMakeModules)

set (CMAKE_INCLUDE_CURRENT_DIR ON)

INCLUDE (CheckFunctionExists)
INCLUDE (CheckIncludeFiles)
INCLUDE (CheckTypeSize)

# -DEVHTP_DISABLE_SSL:STRING=ON
OPTION(EVHTP_DISABLE_SSL        "Disable ssl support"           OFF)

# -DEVHTP_DISABLE_EVTHR:STRING=ON
OPTION(EVHTP_DISABLE_EVTHR      "Disable evthread support"      OFF)

# -DEVHTP_DISABLE_REGEX:STRING=ON
OPTION(EVHTP_DISABLE_REGEX      "Disable regex support"         OFF)

# -DEVHTP_USE_DEFER_ACCEPT:STRING=ON
OPTION(EVHTP_USE_DEFER_ACCEPT   "Enable TCP_DEFER_ACCEPT"       OFF)

if (EVHTP_USE_DEFER_ACCEPT)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_DEFER_ACCEPT")
    set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DUSE_DEFER_ACCEPT")
    set (CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DUSE_DEFER_ACCEPT")
endif(EVHTP_USE_DEFER_ACCEPT)

include(BaseConfig)

find_package(LibEvent REQUIRED)
if (NOT LIBEVENT_OPENSSL_LIBRARY)
    set (EVHTP_DISABLE_SSL ON)
endif()
if (NOT EVHTP_DISABLE_SSL)
    find_package (OpenSSL)
endif()
find_path(LIBEVENT_INCLUDE_DIR event2/event.h REQUIRED)

if (NOT EVHTP_DISABLE_REGEX)
    find_library (HAS_SYS_ONIG onig)
    if (NOT HAS_SYS_ONIG)
        set (ONIG_SOURCES "")
        set (ONIG_LIBS "")
        set (ONIG_INCLUDE_DIR "")
        set (EVHTP_DISABLE_REGEX ON)
    else()
        message("-- Using system libonig")
        set (ONIG_SOURCES "")
        set (ONIG_LIBS ${HAS_SYS_ONIG})
        set (ONIG_INCLUDE_DIR "")
    endif (NOT HAS_SYS_ONIG)
else()
    set (ONIG_SOURCES "")
    set (ONIG_LIBS "")
    set (ONIG_INCLUDE_DIR "")
endif()

find_package (LibMemcached REQUIRED)
find_package (JPEG REQUIRED)
find_package (GIF REQUIRED)
find_package (PNG REQUIRED)
#find_package (WebP REQUIRED)
#find_package (ImageMagick COMPONENTS MagickCore REQUIRED)
#find_package (ImageMagick COMPONENTS MagickWand REQUIRED)

set (DEPS_SOURCE_DIR "${PROJECT_SOURCE_DIR}/../deps")

message (${OPENSSL_INCLUDE_DIR})
message (${OPENSSL_LIBRARIES})

if (NOT OPENSSL_FOUND)
    message ("Diabling SSL")
    set (EVHTP_DISABLE_SSL ON)
    set (OPENSSL_CRYPTO_LIBRARY "")
    set (OPENSSL_INCLUDE_DIR "")
    set (OPENSSL_LIBRARIES "")
    set (LIBEVENT_OPENSSL_LIBRARY "")
endif()

set (WEBP_INCLUDE_DIRS ${DEPS_SOURCE_DIR}/libwebp-0.4.3/src)
set (WEBP_LIBRARIES ${DEPS_SOURCE_DIR}/libwebp-0.4.3/src/.libs/libwebp.a)

set (ImageMagick_INCLUDE_DIRS ${DEPS_SOURCE_DIR}/ImageMagick-6.9.1-10)
set (ImageMagick_MagickCore_LIBRARY ${DEPS_SOURCE_DIR}/ImageMagick-6.9.1-10/wand/.libs/libMagickWand-6.Q8.a)
set (ImageMagick_MagickWand_LIBRARY ${DEPS_SOURCE_DIR}/ImageMagick-6.9.1-10/magick/.libs/libMagickCore-6.Q8.a)

set (LUAJIT_INCLUDE_DIR ${DEPS_SOURCE_DIR}/LuaJIT-2.0.3/src)
set (LUAJIT_LIBRARIES ${DEPS_SOURCE_DIR}/LuaJIT-2.0.3/src/libluajit.a)

include_directories(
    ${OPENSSL_INCLUDE_DIR}
    ${LIBEVENT_INCLUDE_DIR}
    ${LIBMEMCACHED_INCLUDE_DIRS}
    ${LUAJIT_INCLUDE_DIR}
    ${JPEG_INCLUDE_DIRS}
    ${GIF_INCLUDE_DIRS}
    ${PNG_INCLUDE_DIRS}
    ${WEBP_INCLUDE_DIRS}
    ${ImageMagick_INCLUDE_DIRS}
)

set (ZIMG_EXTERNAL_LIBS
    ${OPENSSL_LIBRARIES}
    ${LIBEVENT_LIBRARY}
    ${LIBEVENT_PTHREADS_LIBRARY}
    ${LIBEVENT_OPENSSL_LIBRARY}
    ${LIBMEMCACHED_LIBRARIES}
    ${LUAJIT_LIBRARIES}
    ${JPEG_LIBRARIES}
    ${GIF_LIBRARIES}
    ${PNG_LIBRARIES}
    ${WEBP_LIBRARIES}
    ${ImageMagick_MagickCore_LIBRARY}
    ${ImageMagick_MagickWand_LIBRARY}
    "m"
    "dl"
)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set (ZIMG_EXTERNAL_LIBS ${ZIMG_EXTERNAL_LIBS} readline)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Qunused-arguments -Wall -DPROJECT_VERSION=${PROJECT_VERSION} -pagezero_size 10000 -image_base 100000000")
endif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

if (NOT ${LIBEVENT_PTHREADS_FOUND})
    set (EVHTP_DISABLE_EVTHR ON)
endif(NOT ${LIBEVENT_PTHREADS_FOUND})

if (NOT EVHTP_DISABLE_EVTHR)
    set (ZIMG_EXTERNAL_LIBS ${ZIMG_EXTERNAL_LIBS} pthread)
    set (LIBEVHTP_SOURCES ${LIBEVHTP_SOURCES} ${DEPS_SOURCE_DIR}/libevhtp/evthr.c)
endif(NOT EVHTP_DISABLE_EVTHR)

set (LIBEVHTP_SOURCES ${LIBEVHTP_SOURCES} ${DEPS_SOURCE_DIR}/libevhtp/evhtp.c ${DEPS_SOURCE_DIR}/libevhtp/htparse.c)
aux_source_directory (${PROJECT_SOURCE_DIR} ZIMG_SOURCES)
aux_source_directory (${DEPS_SOURCE_DIR}/hiredis LIBHIREDIS_SOURCE_DIR)
aux_source_directory (${DEPS_SOURCE_DIR}/cjson LIBCJSON_SOURCE_DIR)
aux_source_directory (${DEPS_SOURCE_DIR}/multipart-parser-c MULTIPART_SOURCE_DIR)
set (SOURCES ${ZIMG_SOURCES} ${LIBEVHTP_SOURCES} ${LIBHIREDIS_SOURCE_DIR} ${LIBCJSON_SOURCE_DIR} ${MULTIPART_SOURCE_DIR})

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 -DDEBUG")
endif (${CMAKE_BUILD_TYPE} STREQUAL "Debug")

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -Wno-unused-function -Wno-deprecated -DMAGICKCORE_HDRI_ENABLE -DMAGICKCORE_QUANTUM_DEPTH=8")
message("Build Type: ${CMAKE_BUILD_TYPE}")
message("Std CFLAGS: ${CMAKE_C_FLAGS}")

include_directories(${DEPS_SOURCE_DIR})
link_directories("/usr/lib" "/usr/local/lib")
add_executable(zimg ${SOURCES})
target_link_libraries(zimg ${ZIMG_EXTERNAL_LIBS})
