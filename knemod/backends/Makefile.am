## Makefile.am for knemo backends

noinst_LTLIBRARIES = libknemo_backends.la

# set the include path for X, qt and KDE
INCLUDES = $(all_includes) -I$(srcdir)/.. -I$(srcdir)/../../common

# the library search path. 
libknemo_backends_la_LDFLAGS = -module -avoid-version $(all_libraries)

# the libraries to link against.
libknemo_backends_la_LIBADD = $(LIB_KIO) $(LIB_KUTILS)

# which sources should be compiled as backends
libknemo_backends_la_SOURCES = backendbase.cpp nettoolsbackend.cpp sysbackend.cpp

# the headers of the backends
noinst_HEADERS = backendbase.h nettoolsbackend.h sysbackend.h

# let automoc handle all of the meta source files (moc)
METASOURCES = AUTO

