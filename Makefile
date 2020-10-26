.PHONY: all dynamorio client clean dependencies

EXECUTABLES = git cmake gnuplot g++ python
K := $(foreach exec,$(EXECUTABLES),\
        $(if $(shell which $(exec)),ok,$(error "No $(exec) in PATH")))

all: dependencies dynamorio client

dependencies: ert

dynamorio:
	git clone https://github.com/DynamoRIO/dynamorio.git
	cd dynamorio; mkdir build; cd build; cmake ..; make -j

ert:
	git clone https://bitbucket.org/berkeleylab/cs-roofline-toolkit ert

client:
	cd client; mkdir build; cd build; cmake ..; make -j

clean:
	rm -rf dynamorio
	rm -rf client/build
