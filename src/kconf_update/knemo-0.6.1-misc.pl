#!/usr/bin/perl -w

# Delete CustomCommands key in the Interface_* groups

use strict;

while ( <> ) {
    next if ( /^$/ );
    if ( /^(\[.+\])$/ ) {
        my $currentGroup = $1;
        if ( $currentGroup =~ /^\[Interface_/ ) {
            print "# DELETE ${currentGroup}CustomCommands\n";
        }
    }
    next;
}
