pdpython3
=========

This repository contains an 'external' (plugin) for [Pure
Data](http://puredata.info) to allow embedding Python programs within Pd program
graphs.  The main 'python' object provided enables loading Python modules,
instantiating Python objects, calling object methods, and receiving method return
values.  

Compiling
---------

System requirements: make, a C compiler, a Pd installation, and Python.
Building under Linux:

    make


Installation
------------

	make install
