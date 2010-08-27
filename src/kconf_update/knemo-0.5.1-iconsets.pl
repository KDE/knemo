#!/usr/bin/perl -w

# Change IconSet values from int to their text values

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

my $newSet;
foreach my $ifaceGroup (@ifaceGroups) {
    my $oldSet = $configFile{$ifaceGroup}{'IconSet'};
    $newSet = $oldSet;
    if ( $oldSet eq "0" ) {
        $newSet = "monitor";
    }
    elsif ( $oldSet eq "1" ) {
        $newSet = "modem";
    }
    elsif ( $oldSet eq "2" ) {
        $newSet = "network";
    }
    elsif ( $oldSet eq "3" ) {
        $newSet = "wireless";
    }

    print "# DELETE ${ifaceGroup}IconSet\n";
    print "${ifaceGroup}\nIconSet=$newSet\n";
}
