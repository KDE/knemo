#!/usr/bin/perl -w

# Delete unused plotter keys

use strict;

while ( <> ) {
    next if ( /^$/ );
    if ( /^(\[.+\])$/ ) {
        my $currentGroup = $1;
        if ( $currentGroup =~ /^\[Plotter_/ ) {
            print "# DELETE ${currentGroup}BottomBar\n";
            print "# DELETE ${currentGroup}ColorBackground\n";
            print "# DELETE ${currentGroup}ColorVLines\n";
            print "# DELETE ${currentGroup}Opacity\n";
        }
    }
    next;
}
