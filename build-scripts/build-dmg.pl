#!/usr/bin/perl
#
# Script to build a .dmg disk image for Mac OS X
#

use strict;
use warnings;

use File::Basename;
use Cwd;
use strict;
use warnings;

my $TOP_DIR = Cwd::realpath(dirname(__FILE__).'/..');

# Check tools required are available
my @TOOLS_REQUIRED = (
  'xcodebuild', 'du', 'hdiutil',
  '/usr/libexec/PlistBuddy',
  'pandoc', 'osascript'
);
foreach my $cmd (@TOOLS_REQUIRED) {
  system("which $cmd > /dev/null") && die "Error: $cmd is not available on this system.";
}


# Build the command-line static binary
safe_chdir($TOP_DIR);
safe_system('./build-scripts/build-static.pl');

# Build the GUI
safe_chdir("$TOP_DIR/macosx");
safe_system(
  'xcodebuild',
  '-project', 'RedStore.xcodeproj',
  '-target', 'RedStore',
  '-configuration', 'Release',
  'clean', 'build'
);


my $app_path = "$TOP_DIR/macosx/build/Release/RedStore.app";
if (!-e $app_path) {
  die "Error: app does not exist: $app_path";
}

my $size;
if  (`du -csk $app_path` =~ /\s*(\d+)\s+total\s*$/si) {
    $size = int $1 * 1.5 / 1024 + 1;
} else {
    die "Failed to parse output from du";
}

# Get the version
my $plist = "$TOP_DIR/macosx/RedStore-Info.plist";
my $version = `/usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' $plist`;
chomp($version);
print "Version number: $version\n";
die "Invalid version number" if (length($version) < 3);

# Create an empty DMG archive
safe_chdir($TOP_DIR);
print "Building $size MB DMG\n";
my $dmgName = "RedStore-$version.dmg";
unlink ($dmgName) if (-e $dmgName);
safe_system(
  'hdiutil', 'create', $dmgName,
  '-megabytes', $size,
  '-fs', 'HFS+',
  '-volname', "RedStore-$version"
);
die "Failed to create empty .dmg disk image." unless (-e $dmgName);

my $dest = "/Volumes/RedStore-$version";
die "Error: already mounted: $dest" if (-e $dest);
safe_system('hdiutil', 'attach', $dmgName);
die "Failed to mount volume" unless (-d $dest);

safe_system('cp', '-Rfp', $app_path, $dest);
safe_system('ln', '-s', '/Applications', "$dest/Applications");

# Convert README.md to RTF
safe_system('pandoc',
  '--standalone',
  '--from', 'markdown',
  '--to', 'rtf',
  '-o', "$dest/README.rtf", 
  "$TOP_DIR/README.md"
);


# Use AppleScript to re-configure the view
open(OSASCRIPT, "|osascript") or die "Failed to open pipe to osascript: $!";
print OSASCRIPT <<EOS;
tell application "Finder"
	tell disk "RedStore-$version"
		open
		set current view of container window to icon view
		set toolbar visible of container window to false
		set statusbar visible of container window to false
		set the bounds of container window to {220, 220, 730, 580}
		set theViewOptions to the icon view options of container window
		set arrangement of theViewOptions to not arranged
		set icon size of theViewOptions to 96
		set position of item "RedStore.app" of container window to {250, 80}
		set position of item "README.rtf" of container window to {100, 250}
		set position of item "Applications" of container window to {400, 250}
		close
		open
		update without registering applications
	end tell
end tell
EOS
close(OSASCRIPT);

safe_system('hdiutil', 'detach', $dest);
die "Failed to un-mount volume" if (-e $dest);

# Compress the disk image
my $tmpName = $dmgName."~";
rename($dmgName, $tmpName);
safe_system(
  'hdiutil', 'convert', $tmpName,
  '-format', 'UDBZ',
  '-o', $dmgName
);
die "Error: Couldn't compress the dmg $dmgName" unless (-e $dmgName);
unlink($tmpName) or die "Failed to delete temporary file.";


sub safe_chdir {
    my ($dir) = @_;
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
