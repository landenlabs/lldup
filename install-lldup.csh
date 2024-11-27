#!/bin/csh -f

xcodebuild -list -project lldup.xcodeproj

# rm -rf DerivedData/
xcodebuild -derivedDataPath ./DerivedData/lldup -scheme lldup  -configuration Release clean build
# xcodebuild -configuration Release -alltargets clean


find ./DerivedData -type f -name lldup -perm +444 -ls

set src=./DerivedData/Build/Products/Release/lldup
echo File=$src
ls -al $src
cp $src ~/opt/bin/
