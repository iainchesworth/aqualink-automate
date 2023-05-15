option(USE_LIBCXX "Use libc++ instead of libstdc++" ON)

if(USE_LIBCXX)
	set(LINUX_FLAGS_CXX "${LINUX_FLAGS_CXX} -stdlib=libc++") # for both compiler & linker
endif(USE_LIBCXX)
