#!/bin/bash          
if [ -z $1 ];then
echo " "
echo "Give a file containing a list of the files to be removed."
echo "Note:One line should not contain more than one file-name."
echo " "
else
if [ -f $1 ];then
count_del=0
while read file
do
#if first character in a line is '#', then that line will not be considered
if [[ ${file:0:1} != "#" ]] ; then #avoid commented lines in the list if any.
if [ -f $file ];then
rm -rf $file
let "count_del++"
fi
fi
done <$1
echo "$0: Number of files deleted = $count_del"
else
echo "List file \"$1\" doesnot exist!"
fi
fi
