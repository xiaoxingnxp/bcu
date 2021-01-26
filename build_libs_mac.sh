cd libcurl/
mkdir build.bcu
cd build.bcu
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DCMAKE_FIND_LIBRARY_SUFFIXES=.a -DCURL_STATICLIB=ON -DCMAKE_FIND_LIBRARY_SUFFIXES=.a -DBUILD_TESTING=OFF -DCURL_DISABLE_LDAP=1 -DCMAKE_USE_OPENSSL=ON -DCURL_ZLIB=OFF -DOPENSSL_ROOT_DIR=${LIBOPENSSL_PREFIX} -DOPENSSL_LIBRARIES=${LIBOPENSSL_PREFIX}/lib
cmake --build . --config Release --clean-first
cd ../../

cd libftdi/
cmake .
make
cd ../