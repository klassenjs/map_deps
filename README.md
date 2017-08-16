Overview:
---------

This is a relatively simple tool to scan a MapServer
(http://www.mapserver.org) map file and list the external resources
that MapServer will need to use the map file.  It does this using
libmapserver so that it inherits the parsing from MapServer itself.
It then walks through the objects created by the parser and lists the
external files it finds.

Currently this includes FONTSETs, SYMBOLSETs, FONTS, TEMPLATE files,
DATA files, ...

However, there is a catch... The map file format is very complex and
may even be Turing complete.  Thus, it is impossible to exhaustively
list all external dependencies in all cases.  If one of these
situations is identified, the program prints a warning and does what
it can.  Examples of what is problematic are some substitutions, calls
to URLs or databases, etc.

Nonetheless, this still produces useful information for many map files.

Keep in mind that currently a working draft rather than a polished product.

Dependencies:
-------------

* C++14 compiler

* C++ Guidelines Support Library
  I used gsl-lite from https://github.com/martinmoene/gsl-lite.git
  commit: 6608ae35aff83aca59bddd26d09341c8bbea8aed

* MapServer 7+, libmapserver and headers.
  I used libmapserver-dev from Debian Stretch

* GDAL Libraries and headers (needed by libmapserver)
  I used libgdal-dev from Debian Stretch


Building:
---------

Edit the paths in the Makefile to match your system.

	make

Install:
--------

There is no install step.

Running:
--------

	./map_deps <path to mapfile>


Example Output:

```
$ ~/map_deps ./basemap.map 
Found Mapserver:
MapServer version 7.0.4 ...
Trying mapfile "./basemap.map"
Required Files:
	Refs:	Filename:
	1	/srv/gm3-demo-data/demo/statedata/county.sbx
	1	/srv/gm3-demo-data/demo/statedata/county.sbn
	1	/srv/gm3-demo-data/demo/statedata/county.shp
	1	/srv/gm3-demo-data/demo/statedata/county.prj
	1	/srv/gm3-demo-data/demo/statedata/county.dbf
	1	/srv/gm3-demo-data/symbols/symbol.sym
	1	/srv/gm3-demo-data/demo/statedata/county.shx
	3	/srv/gm3-demo-data/fonts/Vera.ttf
	1	/srv/gm3-demo-data/fonts/fontset.list
	1	/srv/gm3-demo-data/demo/statedata/basemap.map
	1	/srv/gm3-demo-data/demo/statedata/muni.qix
	1	/srv/gm3-demo-data/demo/statedata/muni.sbn
	2	/srv/gm3-demo-data/fonts/VeraBd.ttf
	1	/srv/gm3-demo-data/demo/statedata/muni.dbf
	1	/srv/gm3-demo-data/demo/statedata/muni.sbx
	1	/srv/gm3-demo-data/demo/statedata/muni.shp
	1	/srv/gm3-demo-data/demo/statedata/muni.shx

```