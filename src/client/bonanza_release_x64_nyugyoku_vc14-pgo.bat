call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" amd64
nmake -f Makefile_release_nyugyoku.vs cl-pgo user32.lib
