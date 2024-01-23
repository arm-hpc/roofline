.PHONY: all dynamorio client clean dependencies

EXECUTABLES = git cmake gnuplot g++ python3
K := $(foreach exec,$(EXECUTABLES),\
        $(if $(shell which $(exec)),ok,$(error "No $(exec) in PATH")))

all: dependencies dynamorio/build/bin64/drrun client

dependencies: ert

dynamorio/build/bin64/drrun:
	git clone https://github.com/DynamoRIO/dynamorio.git
	cd dynamorio; git submodule init; git submodule update; mkdir build; cd build; cmake ..; make -j

ert:
	git clone https://bitbucket.org/berkeleylab/cs-roofline-toolkit ert

client:
	cd client; mkdir build; cd build; cmake ..; make -j

clean:
	rm -rf dynamorio
	rm -rf client/build
