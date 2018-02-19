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
$COCO OLT/OLT.atg -namespace osmscout:olt -frames OLT -o OLT 

sed -i s/\(L\"/\(\"/g OLT/Scanner.cpp
sed -i s/L\'/\'/g OLT/Scanner.cpp
sed -i s/wchar_t/char/g OLT/Scanner.cpp

sed -i s/L\"/\"/g OLT/Parser.cpp

cp OLT/Scanner.h ../include/osmscout/olt/
cp OLT/Scanner.cpp ../src/osmscout/olt

cp OLT/Parser.h ../include/osmscout/olt/
cp OLT/Parser.cpp ../src/osmscout/olt
