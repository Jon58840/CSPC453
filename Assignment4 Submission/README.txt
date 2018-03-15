CPSC 453 - Assignment 4
Ray Tracing

Jonathan Ng
10102824

Fall 2016 - November, 30th, 2016
----------------------------------
NOTES:

This is incomplete, the lighting doesn't work quite right, and all associated things,
i.e. shadows and reflections, and the planes are just some odd kind of weird.

Near the top of the cpp file where the variables are you can change "defaultRecursion"
from 0 to a higher integer. It doesn't appear to actually reflect properly, it just makes
rendering take longer.

----------------------------------

CONTROLS:

Change Scene:
0: Default (Just some random triangle, nothing to do with ray tracing)
1: Scene 1
2: Scene 2
3: Scene 3 (My Scene)

Change Magnification:
Left Arrow Key: Zoom out
Right Arrow Key: Zoom in

S: Save rendered image to file

---------------------------------

OPERATING SYSTEM AND COMPILER:
This assignment was done on the CPSC computers on Linux using the makefile included.
