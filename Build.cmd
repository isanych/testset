call "%VS140COMNTOOLS%..\..\VC\vcvarsall.bat" amd64
cl /EHsc /O2 /GL /GS- "/I%CD%" /DNDEBUG TestSet.cpp /link /OPT:REF /out:TestSet.exe
cl /EHsc /O2 /GL /GS- "/I%CD%" /DNDEBUG /DWRAP_ALLOC TestSet.cpp /link /OPT:REF /out:TestSetAlloc.exe
cl /EHsc /O2 /GL /GS- "/I%CD%" /DNDEBUG /DWRAP_ALLOC /DUSE_PPOL TestSet.cpp /link /OPT:REF /out:TestSetPool.exe
