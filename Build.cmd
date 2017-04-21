call "%VS140COMNTOOLS%..\..\VC\vcvarsall.bat" amd64
del TestSet*.exe
cl /EHsc /O2 /GL /GS- "/I%CD%" /DMSVC_COMPILER /DWIN32 /DNDEBUG TestSet.cpp sdsl-src\*.cpp /link /OPT:REF /out:TestSet.exe
del *.obj
cl /EHsc /O2 /GL /GS- "/I%CD%" /DMSVC_COMPILER /DWIN32 /DNDEBUG /DWRAP_ALLOC TestSet.cpp sdsl-src\*.cpp /link /OPT:REF /out:TestSetAlloc.exe
del *.obj
cl /EHsc /O2 /GL /GS- "/I%CD%" /DMSVC_COMPILER /DWIN32 /DNDEBUG /DWRAP_ALLOC /DUSE_POOL TestSet.cpp sdsl-src\*.cpp /link /OPT:REF /out:TestSetPool.exe
del *.obj
