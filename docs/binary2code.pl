#!/usr/bin/perl


die "Usage: binary2code.pl <filename>\n" unless (scalar(@ARGV) == 1);

my $filename = $ARGV[0];
open(DATA, $filename) or die "Failed to open filename: $!";
while(read(DATA,$data,8)) {

    print "        ";
    foreach(unpack("C*", $data)) {
        print sprintf("0x%2.2x, ", $_); 
    }
    print "\n";

}
