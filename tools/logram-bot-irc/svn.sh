#!/bin/sh

cd ../../../../

PREFIX="trunk/Distro/tools/logram-bot-irc"

rm -rf $PREFIX/svn.txt

svn log -rHEAD > $PREFIX/tmp.txt

rev=$(cat $PREFIX/tmp.txt | grep -P '^r' | cut -d ' ' -f 1)
auth=$(cat $PREFIX/tmp.txt | grep -P '^r' | cut -d ' ' -f 3)
msg=$(cat $PREFIX/tmp.txt | tail -n +4 | head -n -1)

echo $rev >> $PREFIX/svn.txt
echo $auth >> $PREFIX/svn.txt
echo $msg >> $PREFIX/svn.txt

cd $PREFIX
