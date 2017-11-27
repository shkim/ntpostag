# NTPosTag

뉴스트러스트 프로젝트 한국어 형태소 분석기

---

## Dependency library preinstall

### Ubuntu:

```sh
# apt-get install libcpprest-dev libboost-python-dev
```

### macOS:

```sh
$ brew install boost@1.59 libiconv openssl cmake
(as of 2017-09, latest boost (@1.65) is not compatible with cmake 3.9.2)
$ brew reinstall boost-python@1.59 --with-python3 --without-python

$ git clone https://github.com/Microsoft/cpprestsdk.git casablanca
$ cd casablanca
$ mkdir build.debug
$ cd build.debug
$ cmake ../Release \
	-DBOOST_ROOT=/usr/local/opt/boost@1.59 \
	-DBoost_NO_SYSTEM_PATHS=ON \
	-DCPPREST_EXCLUDE_WEBSOCKETS=1 \
	-DBUILD_SAMPLES=OFF \
	-DBUILD_TESTS=OFF \
	-DCMAKE_BUILD_TYPE=Debug 
$ make -j 4

$ export DYLD_LIBRARY_PATH=casablanca/build.debug/Binaries
```
