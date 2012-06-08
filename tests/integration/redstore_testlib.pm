#!/usr/bin/perl
#
# Library of functions to assist with testing RedStore
#

use LWP::UserAgent;
use POSIX ":sys_wait_h";
use File::Basename qw(dirname);
use Cwd;
use Errno;
use warnings;
use strict;

sub new_redstore_client {
  my $ua = LWP::UserAgent->new;
  $ua->timeout(2);
  $ua->max_redirect(0);
  $ua->default_header('Accept' => "*/*");
  $ua->agent('RedStoreTestLib/1 ');
  return $ua;
}

sub start_redstore {
    my $storage = shift || 'memory';
    my $storage_options = shift || undef;
    my $storage_name = shift || 'redstore-test';
    my $storage_new = shift;

    my ($cmd, $pid, $port);
    my $count = 0;

    if ($ENV{REDSTORE}) {
      $cmd = $ENV{REDSTORE};
    } elsif ($ARGV[0]) {
      $cmd = $ARGV[0];
    } else {
      die "Error: REDSTORE environment variable is not set and no command line argument given";
    }

    unless (-e $cmd) {
        die "Error: RedStore command does not exist: $cmd";
    }

    do {
        $port = 10000 + (int(rand(10000) + time()) % 10000);
        $count += 1;

        print "# Starting RedStore on port $port.\n";
        if (!defined($pid = fork())) {
            # fork returned undef, so failed
            die "cannot fork: $!";
        } elsif ($pid == 0) {
            my @args = (
              $cmd,
              '-b', 'localhost',
              '-p', $port,
              '-s', $storage
            );
            push(@args, '-n') if ($storage_new);
            push(@args, '-t', $storage_options) if ($storage_options);
            push(@args, $storage_name);
            print "# ".join(' ', @args)."\n";
            open(STDOUT, ">redstore-test.log") or
              die "Can't redirect STDOUT to log file: $!";
            open(STDERR, ">&", \*STDOUT) or
              die "Can't dup STDOUT: $!";
            exec(@args);
            die "failed to exec redstore: $!";
        }

        sleep(1);
    } while (kill(0,$pid) <= 0 && $count < 10);

    die "Failed to start redstore" if ($count == 10);

    # Give it a little bit longer to get ready
    sleep(1);

    return ($pid, "http://localhost:$port/");
}

sub stop_redstore {
    my ($pid) = @_;
    my $count = 0;

    return unless ($pid);

    while (waitpid($pid,WNOHANG) == 0 && $count < 10) {
        $count += 1;
        if ($count<5) {
            kill(2, $pid);
        } else {
            warn "Sending KILL to $pid\n";
            kill(9, $pid);
        }
        sleep(1);
    }
}

sub is_running {
    my ($pid) = @_;
    ok(kill(0,$pid) > 0, "Process $pid running");
}

sub load_fixture {
    my ($filename, $target_uri) = @_;
    my $fixture = read_fixture($filename);

    my $request = HTTP::Request->new( 'PUT', $target_uri );
    $request->content( $fixture );
    $request->content_length( length($fixture) );
    if ($filename =~ /\.rdf$/) {
      $request->content_type( 'application/rdf+xml' );
    } elsif ($filename =~ /\.ttl$/) {
      $request->content_type( 'application/x-turtle' );
    } elsif ($filename =~ /\.nt$/) {
      $request->content_type( 'text/plain' );
    }

    my $ua = new_redstore_client();
    my $response = $ua->request($request);
    is($response->code, 200, "Loading $filename into $target_uri is successful.");
}

sub fixture_path {
    return Cwd::realpath(dirname(__FILE__).'/'.$_[0]);
}

sub fixture_url {
    return 'file://'.fixture_path($_[0]);
}

sub read_fixture {
    my ($filename) = @_;
    my $data = '';
    open(FILE, fixture_path($filename)) or die "Failed to open file: $!";
    while(<FILE>) { $data .= $_; }
    close(FILE);
    return $data;
}

sub is_valid_xhtml {
    my ($xhtml, $name) = @_;

    is_wellformed_xml($xhtml, $name);

    # FIXME: don't let xmllint download DTDs
    # FIXME: hide "- validates" message
# 	SKIP: {
#         # Check that xmllint is installed
#         if (system("which xmllint > /dev/null")) {
#             skip("xmllint is not available", 1);
#         }
#
#         # Open a pipe to xmllint
#         open(LINT, "|xmllint --noout --valid --schema xhtml1-strict.xsd -")
#             or die "Failed to open pipe to xmllint: $!";
#         print LINT $xhtml;
#         close(LINT);
#
#         if ($?) {
#         	fail($name);
#         } else {
#         	pass($name);
#         }
#     }
}

sub is_wellformed_xml {
    my ($xml, $name) = @_;

	SKIP: {
		eval { require XML::Parser; };
		skip("XML::Parser not installed", 1) if $@;

		my $parser = new XML::Parser(ErrorContext => 2);
    	eval { $parser->parse($xml); };
    	if ($@) {
        	$@ =~ s/at \/.*?$//s;
        	print STDERR "$@\n";
        	fail($name);
    	} else {
        	pass($name);
    	}
    }
}

sub is_wellformed_json {
    my ($json, $name) = @_;

	SKIP: {
		eval { require JSON ; };
		skip("JSON module not installed", 1) if $@;
		skip("JSON module is too old", 1) if (!defined $JSON::VERSION or $JSON::VERSION < 2.0);

    	eval { JSON::from_json($json); };
    	if ($@) {
        	$@ =~ s/at \/.*?$//s;
        	print STDERR "$@\n";
        	fail($name);
    	} else {
        	pass($name);
    	}
    }
}

1;
