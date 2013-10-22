mkdir _Automated
..\..\..\..\Qt\qtbase\bin\moc ..\..\..\Source\Core\ffmpeg_BasicInfo.h -o_Automated\moc_ffmpeg_BasicInfo.cpp
..\..\..\..\Qt\qtbase\bin\moc ..\..\..\Source\Core\ffmpeg_Pictures.h -o_Automated\moc_ffmpeg_Pictures.cpp
..\..\..\..\Qt\qtbase\bin\moc ..\..\..\Source\Core\ffmpeg_Stats.h -o_Automated\moc_ffmpeg_Stats.cpp
..\..\..\..\Qt\qtbase\bin\moc ..\..\..\Source\Core\ffmpeg_Thumbnails.h -o_Automated\moc_ffmpeg_Thumbnails.cpp
..\..\..\..\Qt\qtbase\bin\moc ..\..\..\Source\GUI\mainwindow.h -o_Automated\moc_mainwindow.cpp
..\..\..\..\Qt\qtbase\bin\moc ..\..\..\Source\GUI\PerPicture.h -o_Automated\moc_PerPicture.cpp
..\..\..\..\Qt\qtbase\bin\moc ..\..\..\Source\GUI\Help.h -o_Automated\moc_Help.cpp
..\..\..\..\Qt\qtbase\bin\rcc ..\..\..\Source\Resource\Resources.qrc -o _Automated\qrc_Resources.cpp
..\..\..\..\Qt\qtbase\bin\uic ..\..\..\Source\GUI\mainwindow.ui -o _Automated\ui_mainwindow.h
..\..\..\..\Qt\qtbase\bin\uic ..\..\..\Source\GUI\PerPicture.ui -o _Automated\ui_PerPicture.h
