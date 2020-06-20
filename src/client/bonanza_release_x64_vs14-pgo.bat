call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64
nmake -f Makefile_release.vs cl-pgo user32.lib