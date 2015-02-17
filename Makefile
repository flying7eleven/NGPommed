# Top-level Makefile for pommed

OFLIB ?=

all: pommed

pommed:
	$(MAKE) -C source OFLIB=$(OFLIB)

clean:
	$(MAKE) -C source clean
	rm -f *~

.PHONY: pommed
