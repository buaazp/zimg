include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

INCLUDE (CheckFunctionExists)
INCLUDE (CheckIncludeFiles)
INCLUDE (CheckTypeSize)

CHECK_FUNCTION_EXISTS(alloca C_ALLOCA)
CHECK_FUNCTION_EXISTS(memcmp HAVE_MEMCMP)
 
CHECK_INCLUDE_FILES(alloca.h HAVE_ALLOCA_H)                                                                              
CHECK_INCLUDE_FILES(strings.h HAVE_STRINGS_H)
CHECK_INCLUDE_FILES(string.h HAVE_STRING_H)
CHECK_INCLUDE_FILES(stdlib.h HAVE_STDLIB_H)
CHECK_INCLUDE_FILES(sys/time.h HAVE_SYS_TIME_H)
CHECK_INCLUDE_FILES(sys/times.h HAVE_SYS_TIMES_H)
CHECK_INCLUDE_FILES(unistd.h HAVE_UNISTD_H)
CHECK_INCLUDE_FILES(memory.h HAVE_MEMORY_H)
CHECK_INCLUDE_FILES(stdarg.h HAVE_STDARG_PROTOTYPES)

CHECK_TYPE_SIZE("int" SIZEOF_INT)
CHECK_TYPE_SIZE("long" SIZEOF_LONG)
CHECK_TYPE_SIZE("short" SIZEOF_SHORT)

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(SOURCES regint.h regparse.h regenc.h st.h 
regerror.c regparse.c regext.c regcomp.c regexec.c reggnu.c
regenc.c regsyntax.c regtrav.c regversion.c st.c
regposix.c regposerr.c 
enc/unicode.c enc/ascii.c enc/utf8.c 
enc/utf16_be.c enc/utf16_le.c 
enc/utf32_be.c enc/utf32_le.c 
enc/euc_jp.c enc/sjis.c enc/iso8859_1.c 
enc/iso8859_2.c  enc/iso8859_3.c 
enc/iso8859_4.c  enc/iso8859_5.c 
enc/iso8859_6.c  enc/iso8859_7.c 
enc/iso8859_8.c  enc/iso8859_9.c 
enc/iso8859_10.c enc/iso8859_11.c 
enc/iso8859_13.c enc/iso8859_14.c 
enc/iso8859_15.c enc/iso8859_16.c 
enc/euc_tw.c enc/euc_kr.c enc/big5.c 
enc/gb18030.c enc/koi8_r.c enc/cp1251.c)

                                                                                                                         
add_library(libonig STATIC ${SOURCES})
set_target_properties(libonig PROPERTIES OUTPUT_NAME "libonig")
