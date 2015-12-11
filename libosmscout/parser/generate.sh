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
