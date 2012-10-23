libOmxCore_source_cpp = \
	wrs_omxcore.cpp \
	$(NULL)

libOmxCore_source_h = \
	../inc/khronos/openmax/include/OMX_Audio.h \
	../inc/khronos/openmax/include/OMX_ContentPipe.h \
	../inc/khronos/openmax/include/OMX_Image.h \
	../inc/khronos/openmax/include/OMX_Index.h \
	../inc/khronos/openmax/include/OMX_IVCommon.h \
	../inc/khronos/openmax/include/OMX_Types.h \
	../inc/khronos/openmax/include/OMX_Video.h \
	../inc/khronos/openmax/include/OMX_Component.h \
	../inc/khronos/openmax/include/OMX_Core.h \
	../inc/khronos/openmax/include/OMX_IndexExt.h \
	../inc/khronos/openmax/include/OMX_IntelErrorTypes.h \
	../inc/khronos/openmax/include/OMX_Other.h \
	../inc/khronos/openmax/include/OMX_VideoExt.h \
	$(NULL)

libOmxCore_source_priv_h = \
	$(NULL)
	
libOmxCore_ldflags = \
	-version-info 0:1:0 \
	$(NULL)

lib_LTLIBRARIES			= libOmxCore.la
libOmxCoreincludedir		= ${includedir}/omx
libOmxCoreinclude_HEADERS	= $(libOmxCore_source_h)
noinst_HEADERS			= $(libOmxCore_source_h_priv)
libOmxCore_la_SOURCES		= $(libOmxCore_source_cpp)
libOmxCore_la_LDFLAGS		= $(libOmxCore_ldflags)
libOmxCore_la_CPPFLAGS       	= -I../inc -I../../utils/inc -I../../base/inc -I../inc/khronos/openmax/include

DISTCLEANFILES = Makefile.in
