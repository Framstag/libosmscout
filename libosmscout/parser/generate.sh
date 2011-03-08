#!/bin/sh

# OST (aka OSMScout types)
cococpp OST/OST.atg -namespace osmscout:ost -frames OST -o OST 

sed -i s/\(L\"/\(\"/g OST/Scanner.cpp
sed -i s/L\'/\'/g OST/Scanner.cpp
sed -i s/wchar_t/char/g OST/Scanner.cpp

sed -i s/L\"/\"/g OST/Parser.cpp

cp OST/Scanner.h ../include/osmscout/ost/
cp OST/Scanner.cpp ../src/osmscout/ost

cp OST/Parser.h ../include/osmscout/ost/
cp OST/Parser.cpp ../src/osmscout/ost

# OSS (aka OSMScout style)
cococpp OSS/OSS.atg -namespace osmscout:oss -frames OSS -o OSS

sed -i s/\(L\"/\(\"/g OSS/Scanner.cpp
sed -i s/L\'/\'/g OSS/Scanner.cpp
sed -i s/wchar_t/char/g OSS/Scanner.cpp

sed -i s/L\"/\"/g OSS/Parser.cpp

cp OSS/Scanner.h ../include/osmscout/oss/
cp OSS/Scanner.cpp ../src/osmscout/oss

cp OSS/Parser.h ../include/osmscout/oss/
cp OSS/Parser.cpp ../src/osmscout/oss
