#!/usr/bin/perl
#
# Script to build a static binary of redstore, including its dependancies
#

use File::Basename;
use Cwd;
use strict;
use warnings;

my $TOP_DIR = Cwd::realpath(dirname(__FILE__).'/..');
my $BUILD_DIR = "$TOP_DIR/build";
my $ROOT_DIR = "$TOP_DIR/root";
my $DEFAULT_CONFIGURE_ARGS = "--enable-static --disable-shared --prefix=$ROOT_DIR ".
                             "--disable-gtk-doc --disable-dependency-tracking ".
                             "--disable-rebuilds";

my $packages = [
    {
        'url' => 'http://pkgconfig.freedesktop.org/releases/pkg-config-0.25.tar.gz',
        'md5' => 'a3270bab3f4b69b7dc6dbdacbcae9745',
        'config' => "./configure $DEFAULT_CONFIGURE_ARGS --with-pc-path=${ROOT_DIR}/lib/pkgconfig",
        'checkfor' => ['bin/pkg-config', 'share/aclocal/pkg.m4']
    },
    {
        'dirname' => 'check-0.9.8',
        'url' => 'http://snapshots.aelius.com/check/check-0.9.8-20110416.tar.gz',
        'md5' => '045906b2a7eb0721a4c14f579e5d8dc2',
        'checkfor' => ['bin/checkmk', 'include/check.h', 'lib/libcheck.a']
    },
    {
        'url' => 'http://kent.dl.sourceforge.net/project/mhash/mhash/0.9.9.9/mhash-0.9.9.9.tar.gz',
        'md5' => 'ee66b7d5947deb760aeff3f028e27d25',
        'checkfor' => ['include/mhash.h', 'lib/libmhash.a'],
    },
    {
        # libuuid
        'url' => 'http://www.kernel.org/pub/linux/utils/util-linux/v2.21/util-linux-2.21.2.tar.gz',
        'md5' => 'b228170ecdfce9ced77313d57b21b37c',
        'make' => "cd libuuid && make",
        'install' => "cd libuuid && make install",
        'checkfor' => ['include/uuid/uuid.h', 'lib/libuuid.a', 'lib/pkgconfig/uuid.pc'],
    },
    {
        'url' => 'http://curl.haxx.se/download/curl-7.26.0.tar.bz2',
        'md5' => 'bfa80f01b3d300359cfb4d409b6136a3',
        'config' => "./configure $DEFAULT_CONFIGURE_ARGS ".
                    "--disable-ssh --disable-ldap --disable-ldaps --disable-rtsp ".
                    "--without-librtmp --disable-dict --disable-telnet --disable-pop3 ".
                    "--disable-imap --disable-smtp --disable-manual --without-libssh2",
        'checkfor' => ['include/curl/curl.h', 'lib/libcurl.a', 'lib/pkgconfig/libcurl.pc'],
    },
    {
        'url' => 'http://kent.dl.sourceforge.net/project/pcre/pcre/8.30/pcre-8.30.tar.bz2',
        'md5' => '98e8928cccc945d04279581e778fbdff',
        'config' => "./configure $DEFAULT_CONFIGURE_ARGS --enable-utf8 ".
                    "--enable-unicode-properties --enable-pcregrep-libz ".
                    "--enable-pcregrep-libbz2",
        'checkfor' => ['include/pcre.h', 'lib/libpcre.a', 'lib/pkgconfig/libpcre.pc']
    },
    {
        'url' => 'ftp://ftp.gmplib.org/pub/gmp-5.0.5/gmp-5.0.5.tar.bz2',
        'md5' => '041487d25e9c230b0c42b106361055fe',
        'config' => "./configure $DEFAULT_CONFIGURE_ARGS ABI=32",
        'checkfor' => ['include/gmp.h', 'lib/libgmp.a'],
    },
    {
        'url' => 'http://xmlsoft.org/sources/libxml2-2.8.0.tar.gz',
        'md5' => 'c62106f02ee00b6437f0fb9d370c1093',
        'checkfor' => ['include/libxml2', 'lib/libxml2.a', 'lib/pkgconfig/libxml-2.0.pc'],
    },
    {
        'url' => 'http://xmlsoft.org/sources/libxslt-1.1.26.tar.gz',
        'md5' => 'e61d0364a30146aaa3001296f853b2b9',
        # Hack to fix broken compile on Mac OS 10.5 SDK
        'make' => 'touch libxslt/ansidecl.h && make',
        'checkfor' => ['include/libxslt/xslt.h', 'lib/libxslt.a', 'lib/pkgconfig/libxslt.pc'],
    },
    {
        'url' => 'http://github.com/lloyd/yajl/tarball/2.0.4',
        'md5' => 'ee6208e697c43dcccf798ce80d370379',
        'dirname' => 'lloyd-yajl-fee1ebe',
        'tarname' => 'yajl-2.0.4.tar.gz',
        'config' => "mkdir build && cd build && cmake ..",
        'make' => "cd build && make yajl_s",
        'install' => "cd build/yajl-2.0.4 && ".
                     "cp -Rfv include/yajl ${ROOT_DIR}/include/ && ".
                     "cp -fv lib/libyajl_s.a ${ROOT_DIR}/lib/libyajl.a",
        'checkfor' => ['include/yajl/yajl_common.h', 'lib/libyajl.a'],
    },
    {
        'url' => 'http://www.sqlite.org/sqlite-autoconf-3071300.tar.gz',
        'md5' => 'c97df403e8a3d5b67bb408fcd6aabd8e',
        'checkfor' => ['include/sqlite3.h', 'lib/libsqlite3.a', 'lib/pkgconfig/sqlite3.pc'],
    },
    {
        'url' => 'http://download.oracle.com/berkeley-db/db-4.8.30.tar.gz',
        'md5' => 'f80022099c5742cd179343556179aa8c',
        'config' => "cd build_unix && ../dist/configure $DEFAULT_CONFIGURE_ARGS --disable-java",
        'make' => 'cd build_unix && make',
        'install' => 'cd build_unix && make install',
        'checkfor' => ['include/db.h', 'lib/libdb.a'],
    },
#     {
#         'url' => 'http://www.iodbc.org/downloads/iODBC/libiodbc-3.52.7.tar.gz',
#         'config' => "./configure $DEFAULT_CONFIGURE_ARGS --disable-gui --disable-gtktest",
#         'checkfor' => 'lib/pkgconfig/libiodbc.pc',
#     },
#     {
#         'url' => 'http://ftp.heanet.ie/mirrors/www.mysql.com/Downloads/MySQL-5.1/mysql-5.1.57.tar.gz',
#         'config' => "./configure $DEFAULT_CONFIGURE_ARGS --without-server --without-docs --without-man",
#         'checkfor' => 'lib/mysql/libmysqlclient.la',
#     },
#     {
#         'url' => 'http://ftp2.uk.postgresql.org/sites/ftp.postgresql.org/source/v9.0.3/postgresql-9.0.3.tar.gz',
#         'config' => "./configure --prefix=${ROOT_DIR}",
#         'make' => 'make -C src/bin/pg_config && '.
#                   'make -C src/interfaces/libpq all-static-lib',
#         'install' => 'make -C src/bin/pg_config install && '.
#                      "cp -fv src/include/postgres_ext.h ${ROOT_DIR}/include/ && ".
#                      "cp -fv src/interfaces/libpq/libpq-fe.h ${ROOT_DIR}/include/ && ".
#                      'make -C src/interfaces/libpq install-lib-static',
#         'checkfor' => 'lib/libpq.a',
#     },
    {
        'url' => 'http://download.librdf.org/source/raptor2-2.0.8.tar.gz',
        'md5' => 'ac60858b875aab8fa7917f21a1237aa9',
        'config' => "./configure $DEFAULT_CONFIGURE_ARGS --with-yajl=${ROOT_DIR}",
        'checkfor' => ['lib/libraptor2.a', 'include/raptor2/raptor2.h', 'lib/pkgconfig/raptor2.pc'],
    },
    {
        'url' => 'http://download.librdf.org/source/rasqal-0.9.29.tar.gz',
        'md5' => '49e4b75a0c67465edf55dd20606715fa',
        'config' => "./configure $DEFAULT_CONFIGURE_ARGS ".
                    "--with-regex-library=pcre --with-digest-library=mhash --with-decimal=gmp ".
                    "--enable-raptor2 --enable-query-languages=all",
        'checkfor' => ['include/rasqal/rasqal.h', 'lib/librasqal.a', 'lib/pkgconfig/rasqal.pc'],
    },
    {
        'dirname' => 'redland-1.0.16',
        'url' => 'http://snapshots.aelius.com/redland/redland-1.0.16-20120703.tar.gz',
        'md5' => 'a21f7eb720ed820bfe120c0f27325b30',
        'config' => "./configure $DEFAULT_CONFIGURE_ARGS --enable-raptor2 --disable-modular ".
                    "--with-bdb=${ROOT_DIR} --with-threestore=no --with-mysql=no --with-sqlite=3 ".
                    "--with-postgresql=no --with-virtuoso=no",
        'checkfor' => ['include/redland.h', 'lib/librdf.a', 'lib/pkgconfig/redland.pc'],
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
$ENV{'M4PATH'} = "${ROOT_DIR}/share/aclocal";
$ENV{'PATH'} = "${ROOT_DIR}/bin:/usr/bin:/bin:/sbin";
$ENV{'PKG_CONFIG_PATH'} = "${ROOT_DIR}/lib/pkgconfig";
$ENV{'CLASSPATH'} = '';

# Check tools required are available
my @TOOLS_REQUIRED = ('cmake', 'curl', 'ed', 'make', 'md5sum', 'patch', 'tar');
foreach my $cmd (@TOOLS_REQUIRED) {
  system("which $cmd > /dev/null") && die "Error: $cmd is not available on this system.";
}

# Add extra CFLAGS if this is Mac OS X
if (`uname` =~ /^Darwin/) {
    die "Mac OS X Developer Tools are not available." unless (-e '/Developer/');

    # Build for i386 only for now
    my $SDK_VER = '10.5';
    my $SDK = '/Developer/SDKs/MacOSX10.5.sdk';
    my $ARCHES = '-arch i386';
    my $MINVER = "-mmacosx-version-min=$SDK_VER";
    die "Mac OS X SDK is not available." unless (-e $SDK);

    $ENV{'CFLAGS'} .= " -isysroot $SDK $ARCHES $MINVER";
    $ENV{'LDFLAGS'} .= " -Wl,-syslibroot,$SDK $ARCHES $MINVER";
    $ENV{'CFLAGS'} .= " -force_cpusubtype_ALL";
    $ENV{'LDFLAGS'} .= " -Wl,-headerpad_max_install_names";
    $ENV{'MACOSX_DEPLOYMENT_TARGET'} = $SDK_VER;
    $ENV{'CMAKE_OSX_ARCHITECTURES'} = 'i386';
    $ENV{'CMAKE_OSX_SYSROOT'} = $SDK;

    my $GCC_VER = '4.2';
    $ENV{'CC'} = "/Developer/usr/bin/gcc-$GCC_VER";
    $ENV{"CPP"} = "/Developer/usr/bin/cpp-$GCC_VER";
    $ENV{"CXX"} = "/Developer/usr/bin/g++-$GCC_VER";
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

    if ($pkg->{'alwaysbuild'} or check_installed($pkg) == 0) {
        check_usr_local($pkg);
        download_package($pkg) if (defined $pkg->{'url'});
        check_digest($pkg) if (defined $pkg->{'tarpath'});
        extract_package($pkg) if (defined $pkg->{'tarpath'});
        clean_package($pkg);
        patch_package($pkg);
        config_package($pkg);
        make_package($pkg);
        test_package($pkg);
        install_package($pkg);

        if (check_installed($pkg) == 0) {
            die "Building $pkg->{'name'} failed.";
        }
    }
}

print "Finished compiling:\n";
my @package_names = sort(map {$_->{'name'}} @$packages);
foreach my $name (@package_names) {
    print " * $name\n";
}

if (`uname` =~ /^Darwin/) {
    print "Copying static binary into Xcode project:\n  ";
    safe_system('mkdir', 'macosx/bin') unless (-e 'macosx/bin');
    safe_system('cp', 'src/redstore', 'macosx/bin/redstore-cli');

    # Write out Credits document
    print "Writing Credits.html into Xcode project.\n";
    open(CREDITS, ">macosx/Credits.html") or die "Failed to write to credits file: $!";
    print CREDITS "<!DOCTYPE html>\n";
    print CREDITS "<html>\n";
    print CREDITS "<head><title>RedStore Credits</title></head>\n";
    print CREDITS "<body style='font-family: Tahoma, sans-serif; font-size: 8pt'>\n";
    print CREDITS "  <p>This version of RedStore uses following libraries:</p>\n";
    print CREDITS "  <ul>\n";
    foreach my $name (@package_names) {
        next if ($name eq 'redstore');
        print CREDITS "    <li>$name</li>\n";
    }
    print CREDITS "  </ul>\n";
    print CREDITS "</body>\n";
    print CREDITS "</html>\n";
    close(CREDITS);
}

print "Done.\n";


sub check_installed {
    my ($pkg) = @_;
    my $checkfor = $pkg->{'checkfor'};

    unless (defined $checkfor and $checkfor) {
        die "Don't know how to check if ".$pkg->{'name'}." is built.";
    }

    print "Checking if ".$pkg->{'name'}." is built correctly.\n";
    $checkfor = [$checkfor] unless (ref $checkfor eq 'ARRAY');
    foreach (@$checkfor) {
        my $path = $ROOT_DIR . '/' . $_;
        unless (-e $path) {
            print "  No - $path is missing.\n";
            return 0;
        }
    }

    # Everything exists
    print "  Yes.\n";
    return 1;
}

sub download_package {
    my ($pkg) = @_;

    unless (-f $pkg->{'tarpath'}) {
        safe_chdir();
        print "Downloading: ".$pkg->{'tarname'}."\n";
        safe_system('curl', '-L', '-k', '-o', $pkg->{'tarpath'}, $pkg->{'url'});
    }
}

sub check_digest {
    my ($pkg) = @_;

    print "Checking digest for: ".$pkg->{'tarname'}."\n";
    if ($pkg->{'md5'}) {
        if (`md5sum $pkg->{'tarpath'}` =~ /^([0-9a-f]{32})\s/) {
            if ($pkg->{'md5'} ne $1) {
                warn "MD5 digests don't match.\n";
                warn "  Expected: $pkg->{'md5'}\n";
                warn "  Actual: $1\n";
                exit(-1);
            }
        } else {
            die "  Unable to calculate MD5 digest for downloaded package.\n";
        }
    } else {
        die "  Error: no digest defined.\n";
    }
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
        safe_system('tar', '-jxf', $pkg->{'tarpath'});
    } elsif ($pkg->{'tarname'} =~ /gz$/) {
        safe_system('tar', '-zxf', $pkg->{'tarpath'});
    } else {
        die "Don't know how to decomress archive.";
    }
}

sub clean_package {
    my ($pkg) = @_;

    safe_chdir($pkg->{'dirpath'});
    print "Cleaning: ".$pkg->{'name'}."\n";
    if ($pkg->{'clean'}) {
        system($pkg->{'clean'});
    } else {
        # this is allowed to fail
        system('make', 'clean') if (-e 'Makefile');
    }
}

sub patch_package {
    my ($pkg) = @_;
    if ($pkg->{'patch'}) {
        safe_chdir($pkg->{'dirpath'});
        my $patchfile = $TOP_DIR.'/'.$pkg->{'patch'};
        safe_system("patch -p0 < $patchfile");
    }
}

sub config_package {
    my ($pkg) = @_;

    safe_chdir($pkg->{'dirpath'});
    print "Configuring: ".$pkg->{'name'}."\n";
    if ($pkg->{'config'}) {
        safe_system($pkg->{'config'});
    } else {
        if (-e "./configure") {
          safe_system("./configure $DEFAULT_CONFIGURE_ARGS");
        } elsif (-e "./autogen.sh") {
          safe_system("./autogen.sh $DEFAULT_CONFIGURE_ARGS");
        } else {
          die "Don't know how to configure ".$pkg->{'name'};
        }
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

sub check_usr_local {
    my ($pkg) = @_;
    my $checkfor = $pkg->{'checkfor'};

    $checkfor = [$checkfor] unless (ref $checkfor eq 'ARRAY');
    foreach (@$checkfor) {
        my $path = "/usr/local/$_";
        if ($path =~ m(/lib/)) {
          print "Checking for: $path\n";
          if (-e $path) {
              die "Error: found ".$pkg->{'name'}." installed as $path, this causes linking problems.";
          } else {
              print "  Doesn't exist (good!)\n";
          }
        }
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
