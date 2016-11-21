mkdir _Automated
..\..\..\..\Qt\bin\qmake ..\..\QtCreator\QCTools.pro > nul
nmake /NOLOGO /f Makefile.debug compiler_uic_make_all compiler_rcc_make_all mocables

move /Y ui_*.h _Automated
move /Y debug\* _Automated
