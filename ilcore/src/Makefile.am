libOmxCore_source_cpp = \
	wrs_omxcore.cpp \
	$(NULL)

libOmxCore_source_h = \
	$(NULL)

libOmxCore_source_priv_h = \
	$(NULL)
	
libOmxCore_ldflags = \
	-version-info 0:1:0 \
	$(NULL)

lib_LTLIBRARIES			= libOmxCore.la
libOmxCoreincludedir		= ${includedir}
libOmxCoreinclude_HEADERS	= $(libOmxCore_source_h)
noinst_HEADERS			= $(libOmxCore_source_h_priv)
libOmxCore_la_SOURCES		= $(libOmxCore_source_cpp)
libOmxCore_la_LDFLAGS		= $(libOmxCore_ldflags)
libOmxCore_la_CPPFLAGS       	= -I../inc -I../../utils/inc -I../../base/inc -I../inc/khronos/openmax/include

DISTCLEANFILES = Makefile.in
