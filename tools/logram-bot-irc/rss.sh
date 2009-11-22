#! /bin/sh

rm -f rss.txt

## News ##
wget -q -O tmp.txt http://www.logram-project.org/feeds/latestnews/
cat tmp.txt | sed -n 's#.*<title>\(.*\)</title>.*#\1#p' | head -n 1 | tail -n 1 >> rss.txt
cat tmp.txt | sed -n 's#.*<link>\(.*\)</link>.*#\1#p' | head -n 1 | tail -n 1 >> rss.txt

## Journaux ##
wget -q -O tmp.txt http://www.logram-project.org/feeds/latestjournal/
cat tmp.txt | sed -n 's#.*<title>\(.*\)</title>.*#\1#p' | head -n 1 | tail -n 1 >> rss.txt
# echo >> rss.txt
cat tmp.txt | sed -n 's#.*<link>\(.*\)</link>.*#\1#p' | head -n 1 | tail -n 1 >> rss.txt
# echo >> rss.txt

## Demandes ##
wget -q -O tmp.txt http://www.logram-project.org/feeds/latestask/
cat tmp.txt | sed -n 's#.*<title>\(.*\)</title>.*#\1#p' | head -n 1 | tail -n 1 >> rss.txt
cat tmp.txt | sed -n 's#.*<link>\(.*\)</link>.*#\1#p' | head -n 1 | tail -n 1 >> rss.txt

## Messages ##
wget -q -O tmp.txt http://www.logram-project.org/feeds/latestmsg/
cat tmp.txt | sed -n 's#.*<title>\(.*\)</title>.*#\1#p' | head -n 1 | tail -n 1 >> rss.txt
cat tmp.txt | sed -n 's#.*<link>\(.*\)</link>.*#\1#p' | head -n 1 | tail -n 1 >> rss.txt

## Wiki ##
wget -q -O tmp.txt http://www.logram-project.org/feeds/latestwiki/
cat tmp.txt | sed -n 's#.*<title>\(.*\)</title>.*#\1#p' | head -n 1 | tail -n 1 >> rss.txt
cat tmp.txt | sed -n 's#.*<link>\(.*\)</link>.*#\1#p' | head -n 1 | tail -n 1 >> rss.txt

## Packages ##
wget -q -O tmp.txt http://www.logram-project.org/feeds/latestpackages/
cat tmp.txt | sed -n 's#.*<title>\(.*\)</title>.*#\1#p' | head -n 1 | tail -n 1 >> rss.txt
cat tmp.txt | sed -n 's#.*<link>\(.*\)</link>.*#\1#p' | head -n 1 | tail -n 1 >> rss.txt
