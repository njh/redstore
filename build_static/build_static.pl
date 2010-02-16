#!/usr/bin/perl

use File::Spec;
use strict;
use warnings;

my $TOP_DIR = File::Spec->rel2abs(File::Spec->curdir());
my $ROOT_DIR = File::Spec->rel2abs(File::Spec->curdir()).'/root';

my $packages = [
    {
        'url' => 'http://ftp.de.debian.org/debian/pool/main/p/pkg-config/pkg-config_0.22.orig.tar.gz',
        'md5sum' => 'fd5c547e9d66ba49bc735ccb8c791f2a',
        'checkfor' => 'bin/pkg-config',
    },
    {
        'url' => 'http://kent.dl.sourceforge.net/project/pcre/pcre/8.01/pcre-8.01.tar.gz',
        'checkfor' => 'lib/pkgconfig/libpcre.pc',
    },
    {
        'dirname' => 'sqlite-3.6.22',
        'url' => 'http://www.sqlite.org/sqlite-amalgamation-3.6.22.tar.gz',
        'checkfor' => 'lib/pkgconfig/sqlite3.pc',
    },
#     {
#         'name' => 'bdb',
#         'url' => 'http://ftp.de.debian.org/debian/pool/main/d/db/db_4.8.26.orig.tar.gz',
#         'configure' => 'cd build_unix && ../dist/configure --disable-java --enable-static --disable-shared',
#         'make' => 'cd build_unix && make',
#     },
    # curl
    # mpfr
    # gmp
    # xslt
    # libxml2
    {
        'url' => 'http://download.librdf.org/source/raptor-1.4.21.tar.gz',
        'checkfor' => 'lib/pkgconfig/raptor.pc',
    },
    {
        'url' => 'http://download.librdf.org/source/rasqal-0.9.18.tar.gz',
        'checkfor' => 'lib/pkgconfig/rasqal.pc',
    },
    {
        'url' => 'http://download.librdf.org/source/redland-1.0.10.tar.gz',
        'config' => "./configure --enable-static --disable-shared --prefix=$ROOT_DIR --disable-modular",
        'install' => 'make install-exec install-pkgconfigDATA',
        'checkfor' => 'lib/pkgconfig/redland.pc',
    },
    {
        'url' => 'http://redstore.googlecode.com/files/redstore-0.2.tar.gz',
        'checkfor' => 'bin/redstore',
    },
];

# Reset environment variables
$ENV{'CFLAGS'} = "-I${ROOT_DIR}/include";
$ENV{'LDFLAGS'} = "-L${ROOT_DIR}/lib";
$ENV{'INFOPATH'} = "${ROOT_DIR}/share/info";
$ENV{'MANPATH'} = "${ROOT_DIR}/share/man";
$ENV{'PATH'} = "${ROOT_DIR}/bin:/usr/bin:/bin:/usr/sbin:/sbin";
$ENV{'PKG_CONFIG_PATH'} = "${ROOT_DIR}/lib/pkgconfig";

print "Root directory: $ROOT_DIR\n";
mkdir($ROOT_DIR);
mkdir($ROOT_DIR.'/bin');
mkdir($ROOT_DIR.'/include');
mkdir($ROOT_DIR.'/lib');
mkdir($ROOT_DIR.'/share');

foreach my $pkg (@$packages) {
    unless (defined $pkg->{'tarname'}) {
        ($pkg->{'tarname'}) = ($pkg->{'url'} =~ /([^\/]+)$/);
    }
    unless (defined $pkg->{'dirname'}) {
        ($pkg->{'dirname'}) = ($pkg->{'tarname'} =~ /^([\w\.\-]+[\d\.\-]+\d)/);
        $pkg->{'dirname'} =~ s/_/\-/g;
    }
    
    unless (defined $pkg->{'checkfor'} && -e $ROOT_DIR.'/'.$pkg->{'checkfor'}) {
        download_package($pkg);
        extract_package($pkg);
        #FIXME: clean_package($pkg);
        config_package($pkg);
        make_package($pkg);
        install_package($pkg);
        
        if (defined $pkg->{'checkfor'} && !-e $ROOT_DIR.'/'.$pkg->{'checkfor'}) {
            die "Installing $pkg->{'dirname'} failed.";
        }
    }
}

print "Everything is now compiled.\n";



sub extract_package {
    my ($pkg) = @_;
    if (-e $TOP_DIR.'/'.$pkg->{'dirname'}) {
        print "Deleting old: $pkg->{'dirname'}\n";
        safe_system('rm', '-Rf', $pkg->{'dirname'});
    }
    
    print "Extracting: $pkg->{'tarname'} into $pkg->{'dirname'}\n";
    safe_system('tar', '-zxvf', $TOP_DIR.'/'.$pkg->{'tarname'});
}

sub download_package {
    my ($pkg) = @_;
    unless (-e $TOP_DIR.'/'.$pkg->{'tarname'}) {
        print "Downloading: ".$pkg->{'tarname'}."\n";
        safe_system('curl', '-o', $TOP_DIR.'/'.$pkg->{'tarname'}, $pkg->{'url'});
    }

    # FIXME: check md5sum
}

sub config_package {
    my ($pkg) = @_;

    print "Configuring: ".$pkg->{'dirname'}."\n";
    chdir($TOP_DIR.'/'.$pkg->{'dirname'}) || die "Failed to change directory: $!";
    if ($pkg->{'config'}) {
        safe_system($pkg->{'config'});
    } else {
        safe_system("./configure --enable-static --disable-shared --prefix=$ROOT_DIR");
    }
}

sub make_package {
    my ($pkg) = @_;

    print "Making: ".$pkg->{'dirname'}."\n";
    chdir($TOP_DIR.'/'.$pkg->{'dirname'}) || die "Failed to change directory: $!";
    if ($pkg->{'make'}) {
        safe_system($pkg->{'make'});
    } else {
        safe_system('make');
    }
}

sub install_package {
    my ($pkg) = @_;

    print "Installing: ".$pkg->{'dirname'}."\n";
    chdir($TOP_DIR.'/'.$pkg->{'dirname'}) || die "Failed to change directory: $!";
    if ($pkg->{'make'}) {
        safe_system($pkg->{'install'});
    } else {
        safe_system('make install');
    }
}


sub safe_system {
    my (@cmd) = @_;
    print "Running: ".join(' ',@cmd)."\n";
    if (system(@cmd)) {
        die "Command failed";
    }
}
