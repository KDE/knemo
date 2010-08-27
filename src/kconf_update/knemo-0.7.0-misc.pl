#!/usr/bin/perl -w

# Convert to the new statistics and traffic warning rules

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

# Make sure we don't replace rules if we run this twice
my $doStats=1;
my $doWarn=1;
if ( grep { /^\[StatsRule_/ } keys %configFile ) {
    $doStats=0;
}
if ( grep { /^\[WarnRule_/ } keys %configFile ) {
    $doWarn=0;
}

foreach my $ifaceGroup (@ifaceGroups) {

    my $iface;
    if ( $ifaceGroup =~ /_(.+)\]$/ ) {
        $iface = $1;
    }

    # change the iconset for this interface
    print "# DELETE ${ifaceGroup}BillingMonths\n";
    print "# DELETE ${ifaceGroup}BillingStart\n";
    print "# DELETE ${ifaceGroup}CustomBilling\n";
    print "# DELETE ${ifaceGroup}BillingWarnRxTx\n";
    print "# DELETE ${ifaceGroup}BillingWarnThreshold\n";
    print "# DELETE ${ifaceGroup}BillingWarnType\n";
    print "# DELETE ${ifaceGroup}BillingWarnUnits\n";

    if ( $doStats ) {
        my $statsCount=0;
        my $customBilling = $configFile{$ifaceGroup}{'CustomBilling'};
        if ( $customBilling ) {
            my $billingStart = $configFile{$ifaceGroup}{'BillingStart'};
            my $billingMonths = $configFile{$ifaceGroup}{'BillingMonths'};

            print "[StatsRule_$iface #0]\nPeriodCount=$billingMonths\n";
            print "[StatsRule_$iface #0]\nStartDate=$billingStart\n";
            $statsCount++;
        }
        print "${ifaceGroup}\nStatsRules=${statsCount}\n";
    }

    if ( $doWarn ) {
        my $warnCount=0;
        my $warnThreshold = $configFile{$ifaceGroup}{'BillingWarnThreshold'};
        if ( $warnThreshold ) {
            my $warnDirection = $configFile{$ifaceGroup}{'BillingWarnRxTx'};
            my $trafficUnits = $configFile{$ifaceGroup}{'BillingWarnUnits'};
            my $warnUnits = $configFile{$ifaceGroup}{'BillingWarnType'};

            my $periodCount = 1;
            my $periodUnits = 0;

            if ( 0 == $warnUnits ) {
                $periodUnits = 0;
            }
            elsif ( 1 == $warnUnits )
            {
                $periodUnits = 1;
            }
            elsif ( 2 == $warnUnits ) {
                if ( $doStats ) {
                    # if custom stats rule then warn per billing period
                    $periodUnits = 4;
                }
                else {
                    # otherwise it's per month
                    $periodUnits = 3;
                }
            }
            elsif ( 3 == $warnUnits ) {
                $periodUnits = 0;
                $periodCount = 24;
            }
            elsif ( 4 == $warnUnits ) {
                $periodUnits = 1;
                $periodCount = 7;
            }
            elsif ( 5 == $warnUnits ) {
                $periodUnits = 1;
                $periodCount = 30
            }

            if ( $warnDirection ) {
                $warnDirection = 2;
            }
            else
            {
                $warnDirection = 0;
            }

            print "[WarnRule_$iface #0]\nThreshold=$warnThreshold\n";
            print "[WarnRule_$iface #0]\nTrafficDirection=$warnDirection\n";
            print "[WarnRule_$iface #0]\nTrafficUnits=$trafficUnits\n";
            print "[WarnRule_$iface #0]\nPeriodCount=$periodCount\n";
            print "[WarnRule_$iface #0]\nPeriodUnits=$periodUnits\n";

            $warnCount++;
        }

        print "${ifaceGroup}\nWarnRules=${warnCount}\n";
    }
}
