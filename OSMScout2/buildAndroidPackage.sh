#!/bin/sh

rm -rf android-pkg
make install INSTALL_ROOT=android-pkg

/opt/android-qt5/5.3.0/bin/androiddeployqt --input android-libOSMScout.so-deployment-settings.json --output android-pkg --android-platform android-16 --verbose --install

