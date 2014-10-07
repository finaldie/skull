prefix ?= /usr/local

install: prepare
	cp -R bin/* $(prefix)/bin
	cp -R lib/* $(prefix)/lib
	cp -R etc/* $(prefix)/etc
	cp -R share/* $(prefix)/share

prepare:
	test -d $(prefix)/bin || mkdir -p $(prefix)/bin
	test -d $(prefix)/lib || mkdir -p $(prefix)/lib
	test -d $(prefix)/etc || mkdir -p $(prefix)/etc
	test -d $(prefix)/share || mkdir -p $(prefix)/share

.PHONY: install prepare
