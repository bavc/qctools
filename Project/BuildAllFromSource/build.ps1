###################################################################################################
# build.ps1 - Batch script for building the Windows version of QCTools                            #
#                                                                                                 #
# Script requirements:                                                                            #
# - qctools_AllInclusive source                                                                   #
# - Microsoft Visual Studio environment                                                           #
# - Qt bin directory in the PATH                                                                  #
# - Python3  binary in the PATH                                                                   #
# - pkg-config binary in the PATH                                                                 #
# - Meson binary in the PATH                                                                      #
# - Ninja binary in the PATH                                                                      #
# - Git binary in the PATH                                                                        #
###################################################################################################

# helpers
function Cmd-Result {
    if (-Not $?) {
        Exit(1)
    }
}

function Copy-FFmpegRuntime([String]$Destination) {
    Get-ChildItem "$FFMPEG_DIR\bin\*.dll" | Copy-Item -Destination $Destination -Force
}

# setup environment
$SCRIPT_DIR=$PSScriptRoot
Push-Location "$SCRIPT_DIR\..\..\.."
    $INSTALL_DIR=(Get-Location).Path
Pop-Location
Set-Location $INSTALL_DIR

$ErrorActionPreference="Stop"
$FFMPEG_ARCHIVE_VERSION="Latest"
$FFMPEG_DOWNLOAD_URL="https://mediaarea.net/download/binary/ffmpeg/latest"
$FFMPEG_DIR="$INSTALL_DIR\ffmpeg"
$Env:PKG_CONFIG_PATH="$INSTALL_DIR\output\lib\pkgconfig"

# get dependencies
Write-Output "Get Dependencies"
Get-Content "$SCRIPT_DIR\dependencies.txt" | ForEach-Object {
    $DIR=($_ -Split ':', 2)[0]
    $URL=($_ -Split ':', 2)[1]
    if (-not (Test-Path "$DIR")) {
        New-Item -ItemType Directory -Path "$DIR"
        Push-Location "$DIR"
           curl.exe -LO "$URL" ; Cmd-Result
           tar --extract --strip-components=1 --file="$($URL.Split('/')[-1])" ; Cmd-Result
           Remove-Item -Force -Path "$($URL.Split('/')[-1])"
        Pop-Location
    }
}

# ffmpeg
Write-Output "Download FFmpeg"
if (Test-Path "$FFMPEG_DIR") {
    Remove-Item -Force -Recurse "$FFMPEG_DIR"
}
New-Item -ItemType Directory -Force -Path "$FFMPEG_DIR"
foreach ($FFMPEG_PACKAGE in @("Dev", "Bin")) {
    $FFMPEG_ARCHIVE="FFmpeg_${FFMPEG_PACKAGE}_${FFMPEG_ARCHIVE_VERSION}_Windows_Shared_x64.zip"
    $FFMPEG_URL="${FFMPEG_DOWNLOAD_URL}/${FFMPEG_ARCHIVE}"
    Invoke-WebRequest -Uri $FFMPEG_URL -OutFile "$INSTALL_DIR\$FFMPEG_ARCHIVE" ; Cmd-Result
    Expand-Archive -Path "$INSTALL_DIR\$FFMPEG_ARCHIVE" -DestinationPath "$FFMPEG_DIR" -Force ; Cmd-Result
    Remove-Item -Force "$INSTALL_DIR\$FFMPEG_ARCHIVE"
}
$Env:FFMPEG="$FFMPEG_DIR"

# freetype
Write-Output "Build FreeType"
if (Test-Path -Path freetype\build) {
    Remove-Item -Recurse -Force -Path freetype\build
}
New-Item -ItemType directory -Name freetype\build
Push-Location -Path freetype\build
    meson setup --prefix "$INSTALL_DIR\output" --buildtype=release -Db_vscrt=md -Dbrotli=disabled -Dbzip2=disabled -Dharfbuzz=disabled -Dpng=disabled -Dzlib=internal .. ; Cmd-Result
    ninja install ; Cmd-Result
Pop-Location

# harfbuzz
Write-Output "Build HarfBuzz"
if (Test-Path -Path harfbuzz\build) {
    Remove-Item -Recurse -Force -Path harfbuzz\build
}
New-Item -ItemType directory -Name harfbuzz\build
Push-Location -Path harfbuzz\build
    meson setup --prefix "$INSTALL_DIR\output" --buildtype=release -Db_vscrt=md -Dglib=disabled -Dgobject=disabled -Dcairo=disabled -Dchafa=disabled -Dicu=disabled -Dgraphite=disabled -Dgraphite2=disabled -Dgdi=disabled -Ddirectwrite=disabled -Dcoretext=disabled -Dwasm=disabled -Dtests=disabled -Dintrospection=disabled -Ddocs=disabled -Ddoc_tests=false -Dutilities=disabled .. ; Cmd-Result
    ninja install ; Cmd-Result
Pop-Location

# vulkan-headers
if (Test-Path -Path vulkan-headers\build) {
    Remove-Item -Recurse -Force -Path vulkan-headers\build
}
New-Item -ItemType directory -Name vulkan-headers\build
Push-Location -Path vulkan-headers\build
    cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR\output" .. ; Cmd-Result
    ninja install ; Cmd-Result
Pop-Location

# qwt
Write-Output "Build Qwt"
if (Test-Path -Path qwt\build) {
    Remove-Item -Recurse -Force -Path qwt\build
}
New-Item -ItemType directory -Name qwt\build
Push-Location -Path qwt\build
    git -C .. apply "$SCRIPT_DIR\qwt.patch" ; Cmd-Result
    $Env:QWT_NO_SVG=1
    $Env:QWT_NO_OPENGL=1
    $Env:QWT_NO_DESIGNER=1
    $Env:QWT_NO_EXAMPLES=1
    $Env:QWT_NO_PLAYGROUND=1
    $Env:QWT_NO_TESTS=1
    $Env:QWT_INSTALL_PREFIX="$INSTALL_DIR\output"
    qmake .. ; Cmd-Result
    nmake install ; Cmd-Result
Pop-Location

# qctools
Write-Output "Build QCTools"
if (Test-Path -Path qctools\Project\QtCreator\build) {
    Remove-Item -Recurse -Force -Path qctools\Project\QtCreator\build
}
New-Item -ItemType directory -Name qctools\Project\QtCreator\build
Push-Location -Path qctools\Project\QtCreator\build
    git -C .. apply "$SCRIPT_DIR\qtavplayer-pktinfo.patch" ; Cmd-Result
    git -C .. apply "$SCRIPT_DIR\qtavplayer-hwframe.patch" ; Cmd-Result
    git -C .. apply "$SCRIPT_DIR\qtavplayer-ffmpeg-ma.patch" ; Cmd-Result
    $Env:QWT_ROOT="$INSTALL_DIR/output"
    $Env:FFMPEG="$FFMPEG_DIR"
    qmake QMAKE_CXXFLAGS+=/Zi QMAKE_LFLAGS+=/INCREMENTAL:NO QMAKE_LFLAGS+=/Debug DEFINES+=QT_AVPLAYER_MULTIMEDIA .. ; Cmd-Result
    nmake Release ; Cmd-Result
    windeployqt qctools-gui/release/QCTools.exe ; Cmd-Result
    windeployqt qctools-cli/release/qcli.exe ; Cmd-Result
    Copy-FFmpegRuntime "qctools-gui/release"
    Copy-FFmpegRuntime "qctools-cli/release"
Pop-Location

Write-Output "QCTools binary is in $INSTALL_DIR/qctools/Project/QtCreator/build/qctools-gui"
Write-Output "qcli binary is in $INSTALL_DIR/qctools/Project/QtCreator/build/qctools-cli"
