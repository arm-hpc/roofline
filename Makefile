DYNAMORIO_DIR ?= $(PWD)/dynamorio
DYNAMORIO_BUILD_DIR ?= $(DYNAMORIO_DIR)/build
.PHONY: all dynamorio client clean dependencies depends

EXECUTABLES = git cmake g++ python3
K := $(foreach exec,$(EXECUTABLES),\
        $(if $(shell which $(exec)),ok,$(error "No $(exec) in PATH")))

all: $(DYNAMORIO_BUILD_DIR)/bin64/drrun client

depends:
	sudo apt-get update && sudo apt-get install cmake g++ doxygen git zlib1g-dev libunwind-dev libsnappy-dev liblz4-dev

$(DYNAMORIO_DIR):
	git clone -b sve_categories git@github.com:ericvh/dynamorio.git $(DYNAMORIO_DIR);cd $(DYNAMORIO_DIR);git submodule init; git submodule update

$(DYNAMORIO_BUILD_DIR): $(DYNAMORIO_DIR)
	mkdir $(DYNAMORIO_BUILD_DIR)

$(DYNAMORIO_BUILD_DIR)/bin64/drrun: $(DYNAMORIO_BUILD_DIR)
	cd $(DYNAMORIO_BUILD_DIR); cmake ..; make -j

client:
	cd client; mkdir -p build; cd build; cmake --trace -DDynamoRIO_DIR=$(DYNAMORIO_BUILD_DIR)/cmake ..; make -j

clean:
	rm -rf dynamorio
	rm -rf client/build
