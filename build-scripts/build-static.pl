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
        'url' => 'http://pkgconfig.freedesktop.org/releases/pkg-config-0.23.tar.gz',
        'config' => "./configure $DEFAULT_CONFIGURE_ARGS --with-pc-path=${ROOT_DIR}/lib/pkgconfig",
        'checkfor' => 'bin/pkg-config',
    },
    {
        'url' => 'http://kent.dl.sourceforge.net/project/check/check/0.9.8/check-0.9.8.tar.gz',
        'checkfor' => 'include/check.h',
#        'checkfor' => 'bin/checkmk',
    },
    {
        'url' => 'http://curl.haxx.se/download/curl-7.20.0.tar.gz',
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
        'dirname' => 'sqlite-3.6.22',
        'url' => 'http://www.sqlite.org/sqlite-amalgamation-3.6.22.tar.gz',
        'checkfor' => 'lib/pkgconfig/sqlite3.pc',
    },
    {
        'url' => 'http://www.iodbc.org/downloads/iODBC/libiodbc-3.52.7.tar.gz',
        'config' => "./configure $DEFAULT_CONFIGURE_ARGS --disable-gui --disable-gtktest",
        'checkfor' => 'lib/pkgconfig/libiodbc.pc',
    },
    {
        'url' => 'http://download.oracle.com/berkeley-db/db-4.8.26.tar.gz',
        'config' => "cd build_unix && ../dist/configure $DEFAULT_CONFIGURE_ARGS --disable-java",
        'make' => 'cd build_unix && make',
        'install' => 'cd build_unix && make install',
        'checkfor' => 'lib/libdb.a',
    },
    {
        'url' => 'http://www.mirrorservice.org/sites/ftp.mysql.com/Downloads/MySQL-5.1/mysql-5.1.44.tar.gz',
        'config' => "./configure $DEFAULT_CONFIGURE_ARGS --without-server --without-docs --without-man",
        'checkfor' => 'lib/mysql/libmysqlclient.la',
    },
# FIXME: add support for Postgres
#    {
#        'url' => 'http://www.mirrorservice.org/sites/ftp.postgresql.org/source/v8.4.2/postgresql-8.4.2.tar.gz',
#    },
    {
        'url' => 'http://download.librdf.org/source/raptor-1.4.21.tar.gz',
        'checkfor' => 'lib/pkgconfig/raptor.pc',
    },
    {
        'url' => 'http://download.librdf.org/source/rasqal-0.9.16.tar.gz',
        'config' => "./configure $DEFAULT_CONFIGURE_ARGS '--enable-query-languages=sparql rdql laqrs'",
        'checkfor' => 'lib/pkgconfig/rasqal.pc',
        
    },
    {
        'url' => 'http://download.librdf.org/source/redland-1.0.10.tar.gz',
        'config' => "./configure $DEFAULT_CONFIGURE_ARGS --disable-modular --with-bdb=$ROOT_DIR",
        'checkfor' => 'lib/pkgconfig/redland.pc',
    },
    {
        'url' => 'http://redstore.googlecode.com/files/redstore-0.2.tar.gz',
        'checkfor' => 'bin/redstore',
    },
];

# Reset environment variables
$ENV{'CFLAGS'} = "-O2 -I${ROOT_DIR}/include";
$ENV{'CPPFLAGS'} = "-I${ROOT_DIR}/include";
$ENV{'ASFLAGS'} = "-I${ROOT_DIR}/include";
$ENV{'LDFLAGS'} = "-L${ROOT_DIR}/lib";
$ENV{'ASFLAGS'} = "-I${ROOT_DIR}/include";
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

print "Root directory: $ROOT_DIR\n";
mkdir($ROOT_DIR);
mkdir($ROOT_DIR.'/bin');
mkdir($ROOT_DIR.'/include');
mkdir($ROOT_DIR.'/lib');
mkdir($ROOT_DIR.'/share');

gtkdoc_hack($ROOT_DIR);


foreach my $pkg (@$packages) {
    unless (defined $pkg->{'tarname'}) {
        ($pkg->{'tarname'}) = ($pkg->{'url'} =~ /([^\/]+)$/);
    }
    unless (defined $pkg->{'dirname'}) {
        ($pkg->{'dirname'}) = ($pkg->{'tarname'} =~ /^([\w\.\-]+[\d\.\-]+\d)/);
        $pkg->{'dirname'} =~ s/_/\-/g;
    }
    
    unless (defined $pkg->{'checkfor'}) {
        die "Don't know how to check if ".$pkg->{'dirname'}." is installed.";
    }
    
    unless (-e $ROOT_DIR.'/'.$pkg->{'checkfor'}) {
        download_package($pkg);
        extract_package($pkg);
        config_package($pkg);
        make_package($pkg);
        install_package($pkg);
        
        if (defined $pkg->{'checkfor'} && !-e $ROOT_DIR.'/'.$pkg->{'checkfor'}) {
            die "Installing $pkg->{'dirname'} failed.";
        }
    }
}

print "Finished compiling:\n";
foreach my $pkg (sort {$a->{'dirname'} cmp $b->{'dirname'}} @$packages) {
    print " * ".$pkg->{'dirname'}."\n";
}


sub extract_package {
    my ($pkg) = @_;
    if (-e $BUILD_DIR.'/'.$pkg->{'dirname'}) {
        print "Deleting old: $pkg->{'dirname'}\n";
        safe_system('rm', '-Rf', $pkg->{'dirname'});
    }

    safe_chdir();
    print "Extracting: $pkg->{'tarname'} into $pkg->{'dirname'}\n";
    if ($pkg->{'tarname'} =~ /bz2$/) {
        safe_system('tar', '-jxvf', $BUILD_DIR.'/'.$pkg->{'tarname'});
    } elsif ($pkg->{'tarname'} =~ /gz$/) {
        safe_system('tar', '-zxvf', $BUILD_DIR.'/'.$pkg->{'tarname'});
    } else {
        die "Don't know how to decomress archive.";
    }
}

sub download_package {
    my ($pkg) = @_;
    
    unless (-e $BUILD_DIR.'/'.$pkg->{'tarname'}) {
        safe_chdir();
        print "Downloading: ".$pkg->{'tarname'}."\n";
        safe_system('curl', '-o', $BUILD_DIR.'/'.$pkg->{'tarname'}, $pkg->{'url'});
    }

    # FIXME: check md5sum?
}

sub config_package {
    my ($pkg) = @_;

    safe_chdir($pkg->{'dirname'});
    print "Configuring: ".$pkg->{'dirname'}."\n";
    if ($pkg->{'config'}) {
        safe_system($pkg->{'config'});
    } else {
        safe_system("./configure $DEFAULT_CONFIGURE_ARGS");
    }
}

sub make_package {
    my ($pkg) = @_;

    safe_chdir($pkg->{'dirname'});
    print "Making: ".$pkg->{'dirname'}."\n";
    if ($pkg->{'make'}) {
        safe_system($pkg->{'make'});
    } else {
        safe_system('make');
    }
}

sub install_package {
    my ($pkg) = @_;

    safe_chdir($pkg->{'dirname'});
    print "Installing: ".$pkg->{'dirname'}."\n";
    if ($pkg->{'install'}) {
        safe_system($pkg->{'install'});
    } else {
        safe_system('make','install');
    }
}

sub safe_chdir {
    my $dir = join('/', $BUILD_DIR, @_);
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