.PHONY: mpitoupc

all: mpitoupc

mpitoupc:
	make -C src

clean:
	make -C src clean
