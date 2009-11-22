#!/bin/sh

rm -f /tmp/svnlog.txt

svn log -rHEAD svn://localhost/logram/trunk > /tmp/svnlog.txt

rev=$(cat /tmp/svnlog.txt | grep -P '^r' | cut -d ' ' -f 1)
auth=$(cat /tmp/svnlog.txt | grep -P '^r' | cut -d ' ' -f 3)
msg=$(cat /tmp/svnlog.txt | tail -n +4 | head -n -1)

echo "$rev" > svn.txt
echo "$auth" >> svn.txt
echo "$msg" >> svn.txt

