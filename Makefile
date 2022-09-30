################################################################################
lib.name = pdpython
PYTHON_CFLAGS  := `python-config  --cflags`
PYTHON_LDFLAGS := `python-config  --ldflags`
cflags = ${PYTHON_CFLAGS} -shared
ldlibs = ${PYTHON_LDFLAGS}
class.sources = src/pdpython.c
sources = 
datafiles = \
pdpython-help.pd \
pdpython-help.py \
README.md \
LICENSE

################################################################################
PDLIBBUILDER_DIR=pd-lib-builder/
include $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder
