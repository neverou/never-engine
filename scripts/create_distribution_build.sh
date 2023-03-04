DIST=$PWD/$1

pushd $PWD/..
config=dist ./build.sh

cp -r bin/linux-debug/* $DIST/
cp -r res/* $DIST/

popd
