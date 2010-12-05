#!/usr/bin/perl

use File::Basename;
use Cwd;
use strict;
use warnings;

my $TOP_DIR = Cwd::realpath(dirname(__FILE__).'/..');
my $BUILD_DIR = "$TOP_DIR/build";
my $ROOT_DIR = "$TOP_DIR/root";
my $DEFAULT_CONFIGURE_ARGS = "--enable-static --disable-shared --prefix=$ROOT_DIR ".
                             "--disable-gtk-doc --disable-dependency-tracking";

my $packages = [
    {
        'url' => 'http://pkgconfig.freedesktop.org/releases/pkg-config-0.25.tar.gz',
        'config' => "./configure $DEFAULT_CONFIGURE_ARGS --with-pc-path=${ROOT_DIR}/lib/pkgconfig",
        'checkfor' => 'bin/pkg-config',
    },
    {
        'dirname' => 'check-0.9.8',
        'url' => 'http://www.aelius.com/njh/redstore/check-20100409.tar.gz',
        'checkfor' => 'bin/checkmk',
    },
    {
        'url' => 'http://curl.haxx.se/download/curl-7.21.2.tar.gz',
        'config' => "./configure $DEFAULT_CONFIGURE_ARGS ".
                    "--disable-ssh --disable-ldap --disable-ldaps --disable-rtsp --disable-dict ".
                    "--disable-telnet --disable-pop3 --disable-imap --disable-smtp ".
                    "--disable-manual --without-libssh2",
        'checkfor' => 'lib/pkgconfig/libcurl.pc',
    },
    {
        'url' => 'http://kent.dl.sourceforge.net/project/pcre/pcre/8.01/pcre-8.01.tar.gz',
        'checkfor' => 'lib/pkgconfig/libpcre.pc',
    },
# FIXME: can't build a universal binary of GMP
#     {
#         'url' => 'ftp://ftp.gmplib.org/pub/gmp-4.3.2/gmp-4.3.2.tar.bz2',
#         'config' => "./configure $DEFAULT_CONFIGURE_ARGS ABI=32",
#         'checkfor' => 'lib/libgmp.la',
#     },
    {
        'url' => 'http://xmlsoft.org/sources/libxml2-2.7.6.tar.gz',
        'checkfor' => 'lib/pkgconfig/libxml-2.0.pc',
    },
    {
        'url' => 'http://xmlsoft.org/sources/libxslt-1.1.26.tar.gz',
        'checkfor' => 'lib/pkgconfig/libxslt.pc',
    },
    {
        'url' => 'http://github.com/lloyd/yajl/tarball/1.0.11',
        'dirname' => 'lloyd-yajl-f4baae0',
        'tarname' => 'yajl-1.0.11.tar.gz',
        'config' => "mkdir build && cd build && cmake ..",
        'make' => "cd build && make yajl_s",
        'install' => "cd build/yajl-1.0.11 && ".
                     "cp -Rfv include/yajl ${ROOT_DIR}/include/ && ".
                     "cp -fv lib/libyajl_s.a ${ROOT_DIR}/lib/libyajl.a",
        'checkfor' => 'lib/libyajl.a',
    },
    {
        'dirname' => 'sqlite-3.7.3',
        'url' => 'http://www.sqlite.org/sqlite-amalgamation-3.7.3.tar.gz',
        'checkfor' => 'lib/pkgconfig/sqlite3.pc',
    },
    {
        'url' => 'http://www.iodbc.org/downloads/iODBC/libiodbc-3.52.7.tar.gz',
        'config' => "./configure $DEFAULT_CONFIGURE_ARGS --disable-gui --disable-gtktest",
        'checkfor' => 'lib/pkgconfig/libiodbc.pc',
    },
    {
        'url' => 'http://www.mirrorservice.org/sites/ftp.uk.debian.org/debian/pool/main/d/db/db_4.8.26.orig.tar.gz',
        'config' => "cd build_unix && ../dist/configure $DEFAULT_CONFIGURE_ARGS --disable-java",
        'make' => 'cd build_unix && make',
        'install' => 'cd build_unix && make install',
        'checkfor' => 'lib/libdb.a',
    },
    {
        'url' => 'http://www.mirrorservice.org/sites/ftp.mysql.com/Downloads/MySQL-5.1/mysql-5.1.53.tar.gz',
        'config' => "./configure $DEFAULT_CONFIGURE_ARGS --without-server --without-docs --without-man",
        'checkfor' => 'lib/mysql/libmysqlclient.la',
    },
# FIXME: add support for Postgres
#    {
#        'url' => 'http://www.mirrorservice.org/sites/ftp.postgresql.org/source/v8.4.2/postgresql-8.4.2.tar.gz',
#    },
    {
        'url' => 'http://download.librdf.org/source/raptor2-1.9.1.tar.gz',
        'checkfor' => 'lib/pkgconfig/raptor2.pc',
    },
    {
        'url' => 'http://download.librdf.org/source/rasqal-0.9.21.tar.gz',
        'config' => "./configure $DEFAULT_CONFIGURE_ARGS --enable-raptor2 --enable-query-languages=all",
        'checkfor' => 'lib/pkgconfig/rasqal.pc',
        
    },
    {
        'url' => 'http://download.librdf.org/source/redland-1.0.12.tar.gz',
        'config' => "./configure $DEFAULT_CONFIGURE_ARGS --enable-raptor2 --disable-modular --with-bdb=${ROOT_DIR}",
        'checkfor' => 'lib/pkgconfig/redland.pc',
    },
    {
        'name' => 'redstore',
        'dirpath' => $TOP_DIR,
        'test' => 'make check',
        'checkfor' => 'bin/redstore',
        'alwaysbuild' => 1,
    },
];

# Reset environment variables
$ENV{'CFLAGS'} = "-O2 -I${ROOT_DIR}/include";
$ENV{'CPPFLAGS'} = "-I${ROOT_DIR}/include";
$ENV{'ASFLAGS'} = "-I${ROOT_DIR}/include";
$ENV{'LDFLAGS'} = "-L${ROOT_DIR}/lib";
$ENV{'INFOPATH'} = "${ROOT_DIR}/share/info";
$ENV{'MANPATH'} = "${ROOT_DIR}/share/man";
$ENV{'PATH'} = "${ROOT_DIR}/bin:/usr/bin:/bin";
$ENV{'PKG_CONFIG_PATH'} = "${ROOT_DIR}/lib/pkgconfig";
$ENV{'CLASSPATH'} = '';

# Add extra CFLAGS if this is Mac OS X
if (`uname` =~ /^Darwin/) {
    # Build Universal Binrary for both PPC and i386
    my $SDK = '/Developer/SDKs/MacOSX10.4u.sdk';
    my $ARCHES = '-arch i386 -arch ppc';
    my $MINVER = '-mmacosx-version-min=10.4';
    die "Mac OS X SDK is not available." unless (-e $SDK);

    $ENV{'CFLAGS'} .= " -isysroot $SDK $ARCHES $MINVER";
    $ENV{'LDFLAGS'} .= " -Wl,-syslibroot,$SDK $ARCHES $MINVER";
    $ENV{'CFLAGS'} .= " -force_cpusubtype_ALL";
    $ENV{'LDFLAGS'} .= " -Wl,-headerpad_max_install_names";
    $ENV{'MACOSX_DEPLOYMENT_TARGET'} = '10.4';
    $ENV{'CMAKE_OSX_ARCHITECTURES'} = 'ppc;i386';

    my $GCC_VER = '4.0';
    $ENV{'CC'} = "/usr/bin/gcc-$GCC_VER";
    $ENV{"CPP"} = "/usr/bin/cpp-$GCC_VER";
    $ENV{"CXX"} = "/usr/bin/g++-$GCC_VER";
    die "gcc version $GCC_VER is not available." unless (-e $ENV{'CC'});

    # Not sure why these are required
    $ENV{'CFLAGS'} .= " -I$SDK/usr/include";   
    $ENV{'LDFLAGS'} .= " -L$SDK/usr/lib";
}

$ENV{'CXXFLAGS'} = $ENV{'CFLAGS'};

print "Build directory: $BUILD_DIR\n";
mkdir($BUILD_DIR);

print "Root directory: $ROOT_DIR\n";
mkdir($ROOT_DIR);
mkdir($ROOT_DIR.'/bin');
mkdir($ROOT_DIR.'/include');
mkdir($ROOT_DIR.'/lib');
mkdir($ROOT_DIR.'/share');

gtkdoc_hack($ROOT_DIR);


foreach my $pkg (@$packages) {
    if (defined $pkg->{'url'} and !defined $pkg->{'tarname'}) {
        ($pkg->{'tarname'}) = ($pkg->{'url'} =~ /([^\/]+)$/);
    }
    if (defined $pkg->{'tarname'} and !defined $pkg->{'tarpath'}) {
        $pkg->{'tarpath'} = $BUILD_DIR.'/'.$pkg->{'tarname'};
    }
    if (defined $pkg->{'tarname'} and !defined $pkg->{'dirname'}) {
        ($pkg->{'dirname'}) = ($pkg->{'tarname'} =~ /^([\w\.\-]+[\d\.\-]+\d)/);
        $pkg->{'dirname'} =~ s/_/\-/g;
    }
    if (defined $pkg->{'dirname'} and !defined $pkg->{'dirpath'}) {
        $pkg->{'dirpath'} = $BUILD_DIR.'/'.$pkg->{'dirname'};
    }
    if (defined $pkg->{'dirname'} and !defined $pkg->{'name'}) {
        $pkg->{'name'} = $pkg->{'dirname'};
    }
    
    unless ($pkg->{'alwaysbuild'} or defined $pkg->{'checkfor'}) {
        die "Don't know how to check if ".$pkg->{'name'}." is already built.";
    }
    
    if ($pkg->{'alwaysbuild'} or !-e $ROOT_DIR.'/'.$pkg->{'checkfor'}) {
        download_package($pkg) if (defined $pkg->{'url'});
        extract_package($pkg) if (defined $pkg->{'tarpath'});
        clean_package($pkg);
        config_package($pkg);
        make_package($pkg);
        test_package($pkg);
        install_package($pkg);
        
        if (defined $pkg->{'checkfor'} && !-e $ROOT_DIR.'/'.$pkg->{'checkfor'}) {
            die "Installing $pkg->{'name'} failed.";
        }
    }
}

print "Finished compiling:\n";
foreach my $pkg (sort {$a->{'name'} cmp $b->{'name'}} @$packages) {
    print " * ".$pkg->{'name'}."\n";
}


sub extract_package {
    my ($pkg) = @_;
    if (-e $pkg->{'dirpath'}) {
        print "Deleting old: $pkg->{'dirpath'}\n";
        safe_system('rm', '-Rf', $pkg->{'dirpath'});
    }

    safe_chdir();
    print "Extracting: $pkg->{'tarname'} into $pkg->{'dirpath'}\n";
    if ($pkg->{'tarname'} =~ /bz2$/) {
        safe_system('tar', '-jxvf', $pkg->{'tarpath'});
    } elsif ($pkg->{'tarname'} =~ /gz$/) {
        safe_system('tar', '-zxvf', $pkg->{'tarpath'});
    } else {
        die "Don't know how to decomress archive.";
    }
}

sub download_package {
    my ($pkg) = @_;
    
    unless (-e $pkg->{'tarpath'}) {
        safe_chdir();
        print "Downloading: ".$pkg->{'tarname'}."\n";
        safe_system('curl', '-L', '-k', '-o', $pkg->{'tarpath'}, $pkg->{'url'});
    }
}

sub clean_package {
    my ($pkg) = @_;

    safe_chdir($pkg->{'dirpath'});
    # this is allowed to fail
    system('make', 'clean') if (-e 'Makefile');
}

sub config_package {
    my ($pkg) = @_;

    safe_chdir($pkg->{'dirpath'});
    print "Configuring: ".$pkg->{'name'}."\n";
    if ($pkg->{'config'}) {
        safe_system($pkg->{'config'});
    } else {
        safe_system("./configure $DEFAULT_CONFIGURE_ARGS");
    }
}

sub make_package {
    my ($pkg) = @_;

    safe_chdir($pkg->{'dirpath'});
    print "Making: ".$pkg->{'name'}."\n";
    if ($pkg->{'make'}) {
        safe_system($pkg->{'make'});
    } else {
        safe_system('make');
    }
}

sub test_package {
    my ($pkg) = @_;

    safe_chdir($pkg->{'dirpath'});
    if ($pkg->{'test'}) {
        print "Testing: ".$pkg->{'name'}."\n";
        safe_system($pkg->{'test'});
    }
}

sub install_package {
    my ($pkg) = @_;

    safe_chdir($pkg->{'dirpath'});
    print "Installing: ".$pkg->{'name'}."\n";
    if ($pkg->{'install'}) {
        safe_system($pkg->{'install'});
    } else {
        safe_system('make','install');
    }
}

sub safe_chdir {
    my ($dir) = @_;
    $dir = $BUILD_DIR unless defined $dir;
    print "Changing to: $dir\n";
    chdir($dir) or die "Failed to change directory: $!";
}

sub safe_system {
    my (@cmd) = @_;
    print "Running: ".join(' ',@cmd)."\n";
    if (system(@cmd)) {
        die "Command failed";
    }
}


# HACK to fix bad gtkdoc detection
sub gtkdoc_hack {
    my ($dir) = @_;
    my $script = "$dir/bin/gtkdoc-rebase";
    
    open(SCRIPT, ">$script") or die "Failed to open $script: $!";
    print SCRIPT "#/bin/sh\n";
    close(SCRIPT);
    
    chmod(0755, $script) or die "Failed to chmod 0755 $script: $!";
}
