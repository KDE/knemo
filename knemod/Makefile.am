## Makefile.am for knemo

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/knemod.pot

# this has all of the subdirectories that make will recurse into.  if
# there are none, comment this out
SUBDIRS = . pics

# this is the program that gets installed.  it's name is used for all
# of the other Makefile.am variables
kde_module_LTLIBRARIES = kded_knemod.la

# set the include path for X, qt and KDE
INCLUDES         = $(all_includes) -I../common

# the library search path. 
kded_knemod_la_LDFLAGS = -module -avoid-version $(all_libraries)

# the libraries to link against.
kded_knemod_la_LIBADD = $(LIB_KIO) $(LIB_KUTILS)

# which sources should be compiled for knemo
kded_knemod_la_SOURCES = knemodaemon.cpp knemodaemon.skel interface.cpp \
                         interfaceicon.cpp interfacetray.cpp \
                         interfacemonitor.cpp interfaceupdater.cpp \
                         interfacestatusdlg.ui interfacestatusdialog.cpp \
                         interfacetooltip.cpp signalplotter.cpp \
						 interfacestatistics.cpp

# these are the headers for your project
noinst_HEADERS   = knemodaemon.h interface.h interfaceicon.h \
                   interfacemonitor.h interfaceupdater.h \
                   interfacestatusdialog.h interfacetray.h \
                   interfacetooltip.h signalplotter.h \
				   interfacestatistics.h global.h

# let automoc handle all of the meta source files (moc)
METASOURCES = AUTO

services_DATA = knemod.desktop
servicesdir = $(kde_servicesdir)/kded

mydata_DATA = eventsrc
mydatadir = $(kde_datadir)/knemo
