## Makefile.am for kcm_knemo

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kcm_knemo.pot

# this is the program that gets installed.  it's name is used for all
# of the other Makefile.am variables
kde_module_LTLIBRARIES = kcm_knemo.la

# set the include path for X, qt and KDE
INCLUDES = -I$(srcdir)/../common -I$(srcdir)/../knemod/backends $(all_includes)

# the library search path. 
kcm_knemo_la_LDFLAGS = -module -avoid-version $(all_libraries)

# the libraries to link against.
kcm_knemo_la_LIBADD = $(LIB_KDEUI)

# which sources should be compiled for knemo
kcm_knemo_la_SOURCES = configdialog.cpp configdlg.ui

# these are the headers for your project
noinst_HEADERS   = configdialog.h

# let automoc handle all of the meta source files (moc)
METASOURCES = AUTO

xdg_apps_DATA = kcm_knemo.desktop
