@echo off

rem ***********************************************************************************************
rem * build.bat - Batch script for building Windows version of QCTools                            *
rem *                                                                                             *
rem *Script requirements:                                                                         *
rem * - Microsoft Visual Studio 2015 at the default place                                         *
rem * - qctools_AllInclusive source tree                                                          *
rem * - Qt binaries tree corresponding to the requested build type (static or shared, x86 or x64) *
rem *   in ..\..\..\Qt                                                                            *
rem * - Cygwin directory with bash, sed, make and diffutils in the PATH                           *
rem * - yasm.exe in the PATH if not provided by Cygwin                                            *
rem * Options:                                                                                    *
rem * - /static           - build statically linked binary                                        *
rem * - /target x86|x64   - target arch (default x86)                                             *
rem ***********************************************************************************************

rem *** Init ***
set ARCH=x86
set STATIC=

set OLD_CD="%CD%"
set OLD_PATH=%PATH%
set BUILD_DIR="%~dp0..\..\.."

set CHERE_INVOKING=1

rem *** Initialize bash user files (needed to make CHERE_INVOKING work) ***
bash --login -c "exit"

rem *** Parse command line ***
:cmdline
if not "%1"=="" (
    if /I "%1"=="/static" set STATIC=1
    if /I "%1"=="/target" (
        set ARCH=%2
        shift
    )
    shift
    goto:cmdline
)

if not "%ARCH%"=="x86" if not "%ARCH%"=="x64" (
    echo ERROR: Unknown value for arch %ARCH%
    goto:clean
)

if "%ARCH%"=="x86" set PLATFORM=Win32
if "%ARCH%"=="x64" set PLATFORM=x64

rem *** Get VC tools path ***
call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" %ARCH%

rem *** Build zlib ***
if "%ARCH%"=="x86" (
    cd "%BUILD_DIR%\zlib\contrib\masmx86"
    del *.obj *.lst
    rem sed from cygwin
    sed -i "s/ml \/coff/ml \/safeseh \/coff/g" bld_ml32.bat
    call bld_ml32.bat
) else (
    cd "%BUILD_DIR%\zlib\contrib\masmx64"
    del *.obj *.lst
    call bld_ml64.bat
)

cd "%BUILD_DIR%\zlib\contrib\vstudio\vc14"
if defined STATIC (
    sed -i "s/>MultiThreadedDLL</>MultiThreaded</g" zlibstat.vcxproj
) else (
    sed -i "s/>MultiThreaded</>MultiThreadedDLL</g" zlibstat.vcxproj
)

MSBuild zlibstat.vcxproj /t:Clean;Build /p:Configuration=Release;Platform=%PLATFORM%

rem *** Build ffmpeg ***
cd "%BUILD_DIR%\ffmpeg"

set FFMPEG_CMDLINE=--prefix^=. --enable-gpl --enable-version3 --toolchain^=msvc
set FFMPEG_CMDLINE=%FFMPEG_CMDLINE% --disable-securetransport --disable-videotoolbox --disable-swscale-alpha
set FFMPEG_CMDLINE=%FFMPEG_CMDLINE% --disable-doc --disable-ffplay --disable-ffprobe --disable-ffserver --disable-debug
if defined STATIC (
    set FFMPEG_CMDLINE=%FFMPEG_CMDLINE% --enable-static --disable-shared
) else (
    set FFMPEG_CMDLINE=%FFMPEG_CMDLINE% --enable-shared --disable-static
)

if exist Makefile bash --login -c "make clean uninstall"
bash --login -c "./configure %FFMPEG_CMDLINE%"
bash --login -c "make install"

rem *** Build qwt ***
cd "%BUILD_DIR%\qwt"
rem TODO: Make dynamically linked version of QWT work
if exist Makefile nmake clean

..\Qt\bin\qmake -recursive
nmake Release

rem *** Build QCTools ***
cd "%BUILD_DIR%\qctools\Project\MSVC2015\GUI"
call qt_update.bat

cd "%BUILD_DIR%\qctools\Project\MSVC2015"
if defined STATIC (
    MSBuild /t:Clean;Build /p:Configuration=StaticRelease;Platform=%PLATFORM%
) else (
    MSBuild /t:Clean;Build /p:Configuration=Release;Platform=%PLATFORM%
)

rem *** Cleaning ***
:clean
set PATH=%OLD_PATH%
cd %OLD_CD%
