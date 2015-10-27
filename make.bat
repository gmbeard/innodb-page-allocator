@ECHO OFF
PUSHD build
cls && ^
cl /EHs /Zi /I ..\include /Fo:test.obj /D TRACE_ALLOCS=1 ..\tests\main.cpp /link /out:test.exe
POPD
