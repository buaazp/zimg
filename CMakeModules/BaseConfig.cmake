if (CMAKE_COMPILER_IS_GNUCC)

	set(RSN_BASE_C_FLAGS      "-Wall -fno-strict-aliasing")
	set(CMAKE_C_FLAGS         "${CMAKE_C_FLAGS} ${RSN_BASE_C_FLAGS} -DPROJECT_VERSION=\"${PROJECT_VERSION}\"")
	set(CMAKE_C_FLAGS_DEBUG   "${CMAKE_C_FLAGS_DEBUG} ${RSN_BASE_C_FLAGS} -ggdb")
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${RSN_BASE_C_FLAGS}")

	if(APPLE)
		# Newer versions of OSX will spew a bunch of warnings about deprecated ssl functions,
		# this should be addressed at some point in time, but for now, just ignore them.
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_BSD_SOURCE -Wno-deprecated-declarations")
	elseif(UNIX)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_BSD_SOURCE -D_POSIX_C_SOURCE=200112")
	endif(APPLE)

endif(CMAKE_COMPILER_IS_GNUCC)

if (EVHTP_DISABLE_EVTHR)
       set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DEVHTP_DISABLE_EVTHR")
endif(EVHTP_DISABLE_EVTHR)

if (EVHTP_DISABLE_SSL)
			set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DEVHTP_DISABLE_SSL")
endif(EVHTP_DISABLE_SSL)

if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)

