mkdir _Automated
..\..\..\..\Qt\qtbase\bin\moc ..\..\..\Source\GUI\mainwindow.h -o_Automated\moc_mainwindow.cpp
..\..\..\..\Qt\qtbase\bin\moc ..\..\..\Source\GUI\Help.h -o_Automated\moc_Help.cpp
..\..\..\..\Qt\qtbase\bin\rcc ..\..\..\Source\Resource\Resources.qrc -o _Automated\qrc_Resources.cpp
..\..\..\..\Qt\qtbase\bin\uic ..\..\..\Source\GUI\mainwindow.ui -o _Automated\ui_mainwindow.h
