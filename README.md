# tonegen
Playing around with waveout to make audio demos (demoscene) 

# Code description
tonegen.c is the core code.  It can be built with either GCC or TCC.

Since the output is eventually targeting small technical demos, 
development is targeting tiny C compiler.

mmsystem.h is mmsystem.h from MinGW but slightly altered to remove a
couple of types and allow it to work with TCC's antique headers.

winmm.def is the output of tiny\_impdef winmm.dll

# Compilation instructions - Tiny C Compiler:
`tiny\_impedef C:\Windows\SysWOW64\winmm.dll`

(this step can be skipped if you use the provided winmm.def,
and only needs to be performed once.  On 32-bit Windows, use winmm from System32)

`tcc tonegen.c -lwinmm -o tonegen.exe`

You can also use it as a JIT compiler:

`tcc tonegen.c -lwinmm -run`

(run needs to be the last parameter)

# Compilation instructions - GCC:

`gcc tonegen.c -l winmm -o tonegen.exe`


