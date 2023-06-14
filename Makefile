################################################################################
lib.name = pdpython
PYTHON_CFLAGS  := $(shell python3-config  --cflags)
PYTHON_LDFLAGS := $(shell python3-config  --ldflags)
cflags = $(PYTHON_CFLAGS) -shared
ldlibs = $(PYTHON_LDFLAGS) -lpython3.9

# aaa:
# 	echo $(PYTHON_CFLAGS)
# 	echo $(PYTHON_LDFLAGS)
# 	echo $(cflags)
# 	echo $(ldlibs)


class.sources = pdpython.c
sources = 
datafiles = \
pdpython-help.pd \
pdpython-help.py \
README.md \
LICENSE


################################################################################
PDLIBBUILDER_DIR=pd-lib-builder/
include $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder
