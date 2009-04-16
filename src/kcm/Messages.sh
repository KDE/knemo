#! /bin/sh
$EXTRACTRC configdlg.ui >> rc.cpp
$XGETTEXT *.cpp -o $podir/kcm_knemo.pot
