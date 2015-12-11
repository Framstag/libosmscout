#!/bin/sh

if [ -x "$(command -v Coco)" ]; then
  COCO=Coco
elif  [ -x "$(command -v cococpp)" ]; then   
  COCO=cococpp
else
  echo "No coco implementation found!"
  exit 1  
fi

echo "Using Coco command '${COCO}'..."

# OSS (aka OSMScout style)
$COCO OSS/OSS.atg -trace AFGIJPSX -namespace osmscout:oss -frames OSS -o OSS

sed -i s/\(L\"/\(\"/g OSS/Scanner.cpp
sed -i s/L\'/\'/g OSS/Scanner.cpp
sed -i s/wchar_t/char/g OSS/Scanner.cpp

sed -i s/L\"/\"/g OSS/Parser.cpp

cp OSS/Scanner.h ../include/osmscout/oss/
cp OSS/Scanner.cpp ../src/osmscout/oss

cp OSS/Parser.h ../include/osmscout/oss/
cp OSS/Parser.cpp ../src/osmscout/oss
