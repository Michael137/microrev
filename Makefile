include config.inc.defaults

all: build

.PHONY:
full: clean setup cmake

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
		cmake -DCMAKE_INSTALL_PREFIX=$(MREV_INSTALL_PREFIX) \
			../src && \
		make -j4 && \
		make install

.PHONY:
build:
	$(MAKE) -C build/
	$(MAKE) -C build/ install
