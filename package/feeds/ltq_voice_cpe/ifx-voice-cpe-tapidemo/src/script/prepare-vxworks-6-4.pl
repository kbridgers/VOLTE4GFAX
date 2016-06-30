#!/usr/bin/perl
################################################################################
# This script goes through the directory hierarchy and copies the VxWorks 6.4  #
# related project files found in any */vxworks_6.4/ folder to its parent		 #
# folder i.e. one level up.																	 #
################################################################################

use File::Find;
use File::Copy;
use Cwd 'abs_path';
use strict;
use Cwd;

my @pathtab;
my @files = (".project", ".wrmakefile", ".wrproject");
my @rmfiles = (".cproject", ".project", ".wrmakefile", ".wrproject");

find(\&d, cwd);

sub d {
   my $item = $File::Find::name;

   if ($item =~ "vxworks_6.4\$" )
   {
      push (@pathtab, $item);
#     printf "$item\n";
   }
} d;

# printf "@pathtab \n";

foreach my $dir (@pathtab) {
# Check whether the listed project files @files exist in folder $dir
   foreach my $file (@files) {
      my $found = 0;
      opendir DIR, $dir or die "Can't open directory $dir: $!\n";
      while (my $searchfile= readdir DIR) {
         if ($file eq $searchfile) {
            $found = 1;
            last;
         }
      }
      if ($found == 0) {
         # skip the error message as .wrmakefile does exist for the romfs projects
         if (!(($dir =~ "romfs") && ($file == ".wrmakefile"))) {
            printf "Can't find file $file in $dir \n";
         }
      }
   }
}

# Remove any potential occurance of the project files in the <top-level> package
# folder, i.e. current folder.
foreach my $dir (@pathtab) {
   my @temp = split("/", $dir,);
   pop @temp;
   my $tmp = join("/", @temp);
#  printf "cleaning $tmp \n";
   foreach my $file (@rmfiles) {
      my $rem = join("/",$tmp,$file);
      unlink($rem);
   }
}

# Copy the project files to the <top-level> package folder, i.e. current folder
foreach my $dir (@pathtab) {
   foreach my $file (@files) {
      my $fromfile = "$dir/$file";
      my @temp = split("/", $dir,);
      pop @temp;
      my $tmp = join("/", @temp);
      my $tofile = "$tmp/$file";
      my $ret = copy($fromfile, $tofile);
      if ($ret eq 0) {
         # skip the error message as .wrmakefile does exist for the romfs projects
         if (!(($tofile =~ "romfs") && ($file == ".wrmakefile"))) {
            printf "File $fromfile cannot be copied to $tofile.";
         }
      }
      else {
         chmod 0666, $tofile or die "Couldn't chmod $tofile: $!";
      }
   }
}
