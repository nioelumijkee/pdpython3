################################################################################
lib.name = pdpython
PYTHON_CFLAGS  := $(shell python3-config  --cflags)
PYTHON_LDFLAGS := $(shell python3-config  --ldflags)
PYTHON_VER := $(shell python3 --version |\
	 awk '{ print $$2 }' |\
	 awk -F'.' '{ print "python" $$1 "." $$2 }')
cflags = $(PYTHON_CFLAGS) -shared
ldlibs = $(PYTHON_LDFLAGS) -l$(PYTHON_VER)
class.sources = pdpython.c
sources = 
datafiles =

################################################################################
PDLIBBUILDER_DIR=pd-lib-builder/
include $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder
