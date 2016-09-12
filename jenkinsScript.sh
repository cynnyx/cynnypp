./deps.sh
rm -rf ./build
mkdir build
cd build
LIBRARY_TYPE=all cmake ..
make 
./test/bin/test_all --reporter junit --out junit.xml
