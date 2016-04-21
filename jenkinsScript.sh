./deps.sh
rm -rf ./build
mkdir build
cd build
cmake ..
make 
./test/bin/test_all --reporter junit --out junit.xml
rm -rf *
export CXX=/home/jenkins/atlas/bin/g++
export CC=/home/jenkins/atlas/bin/gcc
export JENKINS=1
export LD_LIBRARY_PATH=/home/jenkins/atlas/lib:$LD_LIBRARY_PATH;
cmake ..
make
./test/bin/test_all --reporter junit --out junit_new.xml
