#!/bin/sh

COCO=cococpp

# OST (aka OSMScout types)
$COCO OST/OST.atg -namespace osmscout:ost -frames OST -o OST 

sed -i s/\(L\"/\(\"/g OST/Scanner.cpp
sed -i s/L\'/\'/g OST/Scanner.cpp
sed -i s/wchar_t/char/g OST/Scanner.cpp

sed -i s/L\"/\"/g OST/Parser.cpp

cp OST/Scanner.h ../include/osmscout/ost/
cp OST/Scanner.cpp ../src/osmscout/ost

cp OST/Parser.h ../include/osmscout/ost/
cp OST/Parser.cpp ../src/osmscout/ost
