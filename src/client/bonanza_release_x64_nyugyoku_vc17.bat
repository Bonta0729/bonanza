call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64
nmake -f Makefile_release_nyugyoku.vs cl user32.lib
