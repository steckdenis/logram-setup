#! /bin/sh

rm -f feeds.txt

## News ##
wget -q -O tmp.txt http://www.logram-project.org/feeds/latestnews/
cat tmp.txt | sed -n 's#.*<title>\(.*\)</title>.*#\1#p' | head -n 1 | tail -n 1 >> feeds.txt
cat tmp.txt | sed -n 's#.*<link>\(.*\)</link>.*#\1#p' | head -n 1 | tail -n 1 >> feeds.txt

## Journaux ##
wget -q -O tmp.txt http://www.logram-project.org/feeds/latestjournal/
cat tmp.txt | sed -n 's#.*<title>\(.*\)</title>.*#\1#p' | head -n 1 | tail -n 1 >> feeds.txt
echo >> feeds.txt
cat tmp.txt | sed -n 's#.*<link>\(.*\)</link>.*#\1#p' | head -n 1 | tail -n 1 >> feeds.txt
echo >> feeds.txt

## Demands ##
wget -q -O tmp.txt http://www.logram-project.org/feeds/latestask/
cat tmp.txt | sed -n 's#.*<title>\(.*\)</title>.*#\1#p' | head -n 1 | tail -n 1 >> feeds.txt
cat tmp.txt | sed -n 's#.*<link>\(.*\)</link>.*#\1#p' | head -n 1 | tail -n 1 >> feeds.txt

## Messages ##
wget -q -O tmp.txt http://www.logram-project.org/feeds/latestmsg/
cat tmp.txt | sed -n 's#.*<title>\(.*\)</title>.*#\1#p' | head -n 1 | tail -n 1 >> feeds.txt
cat tmp.txt | sed -n 's#.*<link>\(.*\)</link>.*#\1#p' | head -n 1 | tail -n 1 >> feeds.txt

## Wiki ##
wget -q -O tmp.txt http://www.logram-project.org/feeds/latestwiki/
cat tmp.txt | sed -n 's#.*<title>\(.*\)</title>.*#\1#p' | head -n 1 | tail -n 1 >> feeds.txt
cat tmp.txt | sed -n 's#.*<link>\(.*\)</link>.*#\1#p' | head -n 1 | tail -n 1 >> feeds.txt
