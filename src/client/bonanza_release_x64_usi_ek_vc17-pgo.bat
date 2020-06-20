call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64
nmake -f Makefile_release_usi_ek.vs cl-pgo user32.lib
