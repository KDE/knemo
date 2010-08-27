#!/usr/bin/perl -w

# Delete unused geometry keys in the interface_* groups

use strict;

while ( <> ) {
    next if ( /^$/ );
    if ( /^(\[.+\])$/ ) {
        my $currentGroup = $1;
        if ( $currentGroup =~ /^\[Interface_/ ) {
            print "# DELETE ${currentGroup}PlotterGeometry\n";
            print "# DELETE ${currentGroup}StatisticsGeometry\n";
            print "# DELETE ${currentGroup}StatusGeometry\n";
        }
    }
    next;
}

