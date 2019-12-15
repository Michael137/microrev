all: clean setup cmake

.PHONY:
clean:
	rm -rf build/
	rm -rf install/

.PHONY:
setup:
	mkdir -p build
	mkdir -p install

.PHONY:
cmake:
	cd build && \
		cmake -DPAPI_DIR=/home/gardei/Git/microrev/src/cmake -DCMAKE_INSTALL_PREFIX=/home/gardei/Git/microrev/install \
			../src && \
		make -j4 && \
		make install
