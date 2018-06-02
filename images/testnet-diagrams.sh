#!/bin/sh

circo ring.dot -Tpng -oring.png
circo star.dot -Tpng -ostar.png
fdp mesh.dot -Tpng -omesh.png
