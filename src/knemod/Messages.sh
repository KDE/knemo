#! /bin/sh
$XGETTEXT `find -name \*.cpp -o -name \*.h` *.cpp *.h -o $podir/knemo.pot
