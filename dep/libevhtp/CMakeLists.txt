cmake_minimum_required(VERSION 2.8)
project(reason)

set(PROJECT_MAJOR_VERSION 1)
set(PROJECT_MINOR_VERSION 2)
set(PROJECT_PATCH_VERSION 9)

set (PROJECT_VERSION ${PROJECT_MAJOR_VERSION}.${PROJECT_MINOR_VERSION}.${PROJECT_PATCH_VERSION})
set (CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMakeModules)

INCLUDE (CheckFunctionExists)
INCLUDE (CheckIncludeFiles)
INCLUDE (CheckTypeSize)

CHECK_FUNCTION_EXISTS(alloca  C_ALLOCA)
CHECK_FUNCTION_EXISTS(memcmp  HAVE_MEMCMP)
CHECK_FUNCTION_EXISTS(strndup HAVE_STRNDUP)
CHECK_FUNCTION_EXISTS(strnlen HAVE_STRNLEN)

CHECK_INCLUDE_FILES(alloca.h HAVE_ALLOCA_H)
CHECK_INCLUDE_FILES(strings.h HAVE_STRINGS_H)
CHECK_INCLUDE_FILES(string.h HAVE_STRING_H)
CHECK_INCLUDE_FILES(stdlib.h HAVE_STDLIB_H)
CHECK_INCLUDE_FILES(sys/time.h HAVE_SYS_TIME_H)
CHECK_INCLUDE_FILES(sys/times.h HAVE_SYS_TIMES_H)
CHECK_INCLUDE_FILES(unistd.h HAVE_UNISTD_H)
CHECK_INCLUDE_FILES(memory.h HAVE_MEMORY_H)
CHECK_INCLUDE_FILES(stdarg.h HAVE_STDARG_PROTOTYPES)
CHECK_INCLUDE_FILES(sys/tree.h HAVE_SYS_TREE)
CHECK_INCLUDE_FILES(sys/queue.h HAVE_SYS_QUEUE)
CHECK_INCLUDE_FILES(sys/un.h HAVE_SYS_UN)

CHECK_TYPE_SIZE("int" SIZEOF_INT)
CHECK_TYPE_SIZE("long" SIZEOF_LONG)
CHECK_TYPE_SIZE("short" SIZEOF_SHORT)

if (NOT HAVE_SYS_TREE)
    CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/compat/sys/tree.h.in ${CMAKE_CURRENT_BINARY_DIR}/compat/sys/tree.h)
endif(NOT HAVE_SYS_TREE)

if (NOT HAVE_SYS_QUEUE)
		CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/compat/sys/queue.h.in ${CMAKE_CURRENT_BINARY_DIR}/compat/sys/queue.h)
endif(NOT HAVE_SYS_QUEUE)

if (NOT HAVE_STRNDUP)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNO_STRNDUP")
endif(NOT HAVE_STRNDUP)

if (NOT HAVE_STRNLEN)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNO_STRNLEN")
endif(NOT HAVE_STRNLEN)

if (NOT HAVE_SYS_UN)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNO_SYS_UN")
endif(NOT HAVE_SYS_UN)

# -DEVHTP_DISABLE_SSL:STRING=ON
OPTION(EVHTP_DISABLE_SSL       "Disable ssl support"      OFF)

# -DEVHTP_DISABLE_EVTHR:STRING=ON
OPTION(EVHTP_DISABLE_EVTHR     "Disable evthread support" OFF)

# -DEVHTP_DISABLE_REGEX:STRING=ON
OPTION(EVHTP_DISABLE_REGEX     "Disable regex support"    OFF)

# -DEVHTP_BUILD_SHARED:STRING=ON
OPTION(EVHTP_BUILD_SHARED      "Build shared library too" OFF)

# -DEVHTP_USE_DEFER_ACCEPT:STRING=ON
OPTION(EVHTP_USE_DEFER_ACCEPT  "Enable TCP_DEFER_ACCEPT"  OFF)

if (EVHTP_USE_DEFER_ACCEPT)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_DEFER_ACCEPT")
endif(EVHTP_USE_DEFER_ACCEPT)

SET(CMAKE_INCLUDE_CURRENT_DIR ON)

include(BaseConfig)

message("Build Type: ${CMAKE_BUILD_TYPE}")
message("Std CFLAGS: ${CMAKE_C_FLAGS}")
message("Dbg CFLAGS: ${CMAKE_C_FLAGS_DEBUG}")
message("Rel CFLAGS: ${CMAKE_C_FLAGS_RELEASE}")

find_package(LibEvent REQUIRED)

if (NOT LIBEVENT_OPENSSL_LIBRARY)
	set (EVHTP_DISABLE_SSL ON)
endif()

if (NOT EVHTP_DISABLE_SSL)
	find_package(OpenSSL)
endif()

find_path(LIBEVENT_INCLUDE_DIR event2/event.h REQUIRED)

if (NOT EVHTP_DISABLE_REGEX)
	find_library(HAS_SYS_ONIG onig)
endif()

if (NOT OPENSSL_FOUND)
	message("Diabling SSL")
	set (EVHTP_DISABLE_SSL ON)
	set (OPENSSL_CRYPTO_LIBRARY "")
	set (OPENSSL_INCLUDE_DIR "")
	set (OPENSSL_LIBRARIES "")
	set (LIBEVENT_OPENSSL_LIBRARY "")
endif()

if (NOT EVHTP_DISABLE_REGEX)
	if (NOT HAS_SYS_ONIG)
		CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/oniguruma/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/oniguruma/config.h)
		set(ONIG_SOURCES
			oniguruma/regerror.c
			oniguruma/regparse.c
			oniguruma/regext.c
			oniguruma/regcomp.c
			oniguruma/regexec.c
			oniguruma/reggnu.c
			oniguruma/regenc.c
			oniguruma/regsyntax.c
			oniguruma/regtrav.c
			oniguruma/regversion.c
			oniguruma/st.c
			oniguruma/regposix.c
			oniguruma/regposerr.c
			oniguruma/enc/unicode.c
			oniguruma/enc/ascii.c
			oniguruma/enc/utf8.c
			oniguruma/enc/utf16_be.c
			oniguruma/enc/utf16_le.c
			oniguruma/enc/utf32_be.c
			oniguruma/enc/utf32_le.c
			oniguruma/enc/euc_jp.c
			oniguruma/enc/sjis.c
			oniguruma/enc/iso8859_1.c
			oniguruma/enc/iso8859_2.c
			oniguruma/enc/iso8859_3.c
			oniguruma/enc/iso8859_4.c
			oniguruma/enc/iso8859_5.c
			oniguruma/enc/iso8859_6.c
			oniguruma/enc/iso8859_7.c
			oniguruma/enc/iso8859_8.c
			oniguruma/enc/iso8859_9.c
			oniguruma/enc/iso8859_10.c
			oniguruma/enc/iso8859_11.c
			oniguruma/enc/iso8859_13.c
			oniguruma/enc/iso8859_14.c
			oniguruma/enc/iso8859_15.c
			oniguruma/enc/iso8859_16.c
			oniguruma/enc/euc_tw.c
			oniguruma/enc/euc_kr.c
			oniguruma/enc/big5.c
			oniguruma/enc/gb18030.c
			oniguruma/enc/koi8_r.c
			oniguruma/enc/cp1251.c)
		set (ONIG_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/oniguruma;${CMAKE_CURRENT_SOURCE_DIR}/oniguruma)
	else()
		message("-- Using system libonig")
		set(ONIG_SOURCES "")
		set(ONIG_LIBS ${HAS_SYS_ONIG})
		set(ONIG_INCLUDE_DIR "")
	endif(NOT HAS_SYS_ONIG)
else()
	set(ONIG_SOURCES "")
	set(ONIG_LIBS "")
	set(ONIG_INCLUDE_DIR "")
endif()

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}/compat
	${CMAKE_CURRENT_SOURCE_DIR}/htparse
	${CMAKE_CURRENT_SOURCE_DIR}/evthr
	${CMAKE_CURRENT_SOURCE_DIR}
	${ONIG_INCLUDE_DIR}
	${OPENSSL_INCLUDE_DIR}
	${LIBEVENT_INCLUDE_DIR}
)

set(LIBEVHTP_EXTERNAL_LIBS
		${LIBEVENT_LIBRARY}
		${LIBEVENT_PTHREADS_LIBRARY}
		${LIBEVENT_OPENSSL_LIBRARY}
		${OPENSSL_LIBRARIES}
		${ONIG_LIBS}
)

if (NOT ${LIBEVENT_PTHREADS_FOUND})
	set(EVHTP_DISABLE_EVTHR ON)
endif(NOT ${LIBEVENT_PTHREADS_FOUND})

if (NOT ${LIBEVENT_OPENSSL_FOUND})
	set (EVHTP_DISABLE_SSL ON)
endif(NOT ${LIBEVENT_OPENSSL_FOUND})

set(LIBEVHTP_SOURCES evhtp.c htparse/htparse.c)

if (NOT EVHTP_DISABLE_EVTHR)
	set (LIBEVHTP_EXTERNAL_LIBS ${LIBEVHTP_EXTERNAL_LIBS} pthread)
	set (LIBEVHTP_SOURCES ${LIBEVHTP_SOURCES} evthr/evthr.c)
endif(NOT EVHTP_DISABLE_EVTHR)

if (NOT ${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNDEBUG")
endif (NOT ${CMAKE_BUILD_TYPE} STREQUAL "Debug")

IF (WIN32)
	ADD_DEFINITIONS(-DWIN32)
	ADD_DEFINITIONS(-march=i486)
	find_library (LIB_WS32 ws2_32)
	set (SYS_LIBS ${SYS_LIBS} ${LIB_WS32})
ELSE ()
	if (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
		find_library (LIB_DL dl)
		set (SYS_LIBS ${LIB_DL})
	endif()

	if (NOT APPLE)
		find_library (LIB_RT rt)
		set (SYS_LIBS ${SYS_LIBS} ${LIB_RT})
	endif()

ENDIF (WIN32)

add_custom_target(examples)

if (EVHTP_BUILD_SHARED)
	set (EVHTP_LIBTYPE SHARED)
else()
	set (EVHTP_LIBTYPE STATIC)
endif()

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/evhtp-config.h.in
	${CMAKE_CURRENT_BINARY_DIR}/evhtp-config.h)

add_library(libevhtp ${EVHTP_LIBTYPE} ${LIBEVHTP_SOURCES} ${ONIG_SOURCES})

set_target_properties(libevhtp PROPERTIES OUTPUT_NAME "evhtp")
target_link_libraries(libevhtp ${LIBEVHTP_EXTERNAL_LIBS})

add_executable(test EXCLUDE_FROM_ALL examples/test.c)
add_executable(test_basic EXCLUDE_FROM_ALL examples/test_basic.c)
add_executable(test_vhost EXCLUDE_FROM_ALL examples/test_vhost.c)
add_executable(test_client EXCLUDE_FROM_ALL examples/test_client.c)
add_executable(test_proxy EXCLUDE_FROM_ALL examples/test_proxy.c)

target_link_libraries(test libevhtp ${LIBEVHTP_EXTERNAL_LIBS} ${SYS_LIBS})
target_link_libraries(test_basic libevhtp ${LIBEVHTP_EXTERNAL_LIBS} ${SYS_LIBS})
target_link_libraries(test_vhost libevhtp ${LIBEVHTP_EXTERNAL_LIBS} ${SYS_LIBS})
target_link_libraries(test_client libevhtp ${LIBEVHTP_EXTERNAL_LIBS} ${SYS_LIBS})
target_link_libraries(test_proxy libevhtp ${LIBEVHTP_EXTERNAL_LIBS} ${SYS_LIBS})

add_dependencies(examples test test_basic test_vhost test_client test_proxy)

if (NOT LIB_INSTALL_DIR)
  set (LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib")
endif (NOT LIB_INSTALL_DIR)

if(NOT INCLUDE_INSTALL_DIR)
	set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/include)
endif()

install (TARGETS libevhtp DESTINATION ${LIB_INSTALL_DIR})
install (FILES evhtp.h DESTINATION ${INCLUDE_INSTALL_DIR})
install (FILES htparse/htparse.h DESTINATION ${INCLUDE_INSTALL_DIR})
install (FILES ${CMAKE_CURRENT_BINARY_DIR}/evhtp-config.h DESTINATION ${INCLUDE_INSTALL_DIR})

if (NOT EVHTP_DISABLE_EVTHR)
	install (FILES evthr/evthr.h DESTINATION ${INCLUDE_INSTALL_DIR})
endif()

# oniguruma/onigposix.h

if (NOT EVHTP_DISABLE_REGEX)
		if (NOT HAS_SYS_ONIG)
				install (FILES oniguruma/onigposix.h DESTINATION ${INCLUDE_INSTALL_DIR})
		endif()
endif()

IF (WIN32)
   install (FILES compat/sys/queue.h DESTINATION ${INCLUDE_INSTALL_DIR}/sys)
   install (FILES oniguruma/onigposix.h DESTINATION ${INCLUDE_INSTALL_DIR})
ENDIF (WIN32)
