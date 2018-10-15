#!/usr/bin/perl -w

# Convert deprecated calendar names to numbers

use strict;

my $currentGroup = "";
my %configFile;
while ( <> ) {
    chomp;
    next if ( /^$/ );
    next if ( /^\#/ );
    if ( /^\[/ ) {
        $currentGroup = $_;
        next;
    } elsif ( $currentGroup ne "" ) {
        my ($key,$value) = split /=/;
        $configFile{$currentGroup}{$key}=$value;
    }
}

my @ifaceGroups = grep { /^\[Interface_\S+\]/ } keys %configFile;

foreach my $ifaceGroup (@ifaceGroups) {
    my $calendar = $configFile{$ifaceGroup}{'Calendar'};
    my $calendarSystem = 1;
    if ( $calendar ) {
        if ( $calendar eq "coptic" ) {
            $calendarSystem = 5
        } elsif ($calendar eq "ethiopian") {
            $calendarSystem = 6
        } elsif ($calendar eq "gregorian-proleptic") {
            $calendarSystem = 8
        } elsif ($calendar eq "hebrew") {
            $calendarSystem = 9
        } elsif ($calendar eq "hijri") {
            $calendarSystem = 12
        } elsif ($calendar eq "indian-national") {
            $calendarSystem = 14
        } elsif ($calendar eq "jalali") {
            $calendarSystem = 16
        } elsif ($calendar eq "japanese") {
            $calendarSystem = 19
        } elsif ($calendar eq "julian") {
            $calendarSystem = 21
        } elsif ($calendar eq "minguo") {
            $calendarSystem = 22
        } elsif ($calendar eq "thai") {
            $calendarSystem = 23
        }
        print "# DELETE ${ifaceGroup}Calendar\n";
        print "${ifaceGroup}\nCalendarSystem=${calendarSystem}\n";
    }
}
