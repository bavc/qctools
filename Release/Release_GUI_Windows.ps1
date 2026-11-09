##  Copyright (c) MediaArea.net SARL. All Rights Reserved.
##
##  Use of this source code is governed by a BSD-style license that can
##  be found in the License.html file in the root of the source tree.
##

$ErrorActionPreference = "Stop"

#-----------------------------------------------------------------------
# Setup
$release_directory = $PSScriptRoot
$version = (Get-Content "${release_directory}\..\Project\version.txt" -Raw).Trim()
$arch = "x64"

#-----------------------------------------------------------------------
# Cleanup
$artifact = "${release_directory}\QCTools_${version}_Windows_${arch}_WithoutInstaller"
if (Test-Path "${artifact}") {
    Remove-Item -Force -Recurse "${artifact}"
}

$artifact = "${release_directory}\QCTools_${version}_Windows_${arch}_WithoutInstaller.zip"
if (Test-Path "${artifact}") {
    Remove-Item -Force "${artifact}"
}

$artifact = "${release_directory}\QCTools_${version}_Windows_${arch}_DebugInfo.zip"
if (Test-Path "${artifact}") {
    Remove-Item -Force "${artifact}"
}

$artifact = "${release_directory}\QCTools_${version}_Windows.exe"
if (Test-Path "${artifact}") {
    Remove-Item -Force "${artifact}"
}

#-----------------------------------------------------------------------
# Package GUI
Push-Location "${release_directory}"
    New-Item -ItemType Directory -Path "QCTools_${version}_Windows_${arch}_WithoutInstaller"
    Push-Location -Path "QCTools_${version}_Windows_${arch}_WithoutInstaller"
        Copy-Item -Force -Path "${release_directory}\..\History.txt" .
        Copy-Item -Force -Path "${release_directory}\..\License.html" .
        Copy-Item -Force -Path "${release_directory}\..\Project\QtCreator\build\qctools-gui\release\QCTools.exe" .
        Copy-Item -Force -Path "${release_directory}\..\Project\QtCreator\build\qctools-gui\release\Qt6Core.dll" .
        Copy-Item -Force -Path "${release_directory}\..\Project\QtCreator\build\qctools-gui\release\Qt6Gui.dll" .
        Copy-Item -Force -Path "${release_directory}\..\Project\QtCreator\build\qctools-gui\release\Qt6Multimedia.dll" .
        Copy-Item -Path "${release_directory}\..\Project\QtCreator\build\qctools-gui\release\Qt6MultimediaWidgets.dll" .
        Copy-Item -Force -Path "${release_directory}\..\Project\QtCreator\build\qctools-gui\release\Qt6Network.dll" .
        Copy-Item -Force -Path "${release_directory}\..\Project\QtCreator\build\qctools-gui\release\Qt6OpenGL.dll" .
        Copy-Item -Force -Path "${release_directory}\..\Project\QtCreator\build\qctools-gui\release\Qt6Qml.dll" .
        Copy-Item -Force -Path "${release_directory}\..\Project\QtCreator\build\qctools-gui\release\Qt6QmlModels.dll" .
        Copy-Item -Force -Path "${release_directory}\..\Project\QtCreator\build\qctools-gui\release\Qt6Svg.dll" .
        Copy-Item -Force -Path "${release_directory}\..\Project\QtCreator\build\qctools-gui\release\Qt6Widgets.dll" .
        Copy-Item -Force -Path "${release_directory}\..\Project\QtCreator\build\qctools-gui\release\d3dcompiler_47.dll" .
        Copy-Item -Force -Path "${release_directory}\..\Project\QtCreator\build\qctools-gui\release\opengl32sw.dll" .
        New-Item -ItemType directory -Path "iconengines"
        Push-Location -Path "iconengines"
            Copy-Item -Force -Path "${release_directory}\..\Project\QtCreator\build\qctools-gui\release\\iconengines\qsvgicon.dll" .
        Pop-Location
        New-Item -ItemType directory -Path "imageformats"
            Push-Location -Path "imageformats"
                Copy-Item -Path "${release_directory}\..\Project\QtCreator\build\qctools-gui\release\imageformats\qjpeg.dll" .
                Copy-Item -Path "${release_directory}\..\Project\QtCreator\build\qctools-gui\release\imageformats\qsvg.dll" .
                Copy-Item -Path "${release_directory}\..\Project\QtCreator\build\qctools-gui\release\imageformats\qico.dll" .
        Pop-Location
        New-Item -ItemType directory -Path "multimedia"
        Push-Location -Path "multimedia"
            Copy-Item -Force -Path "${release_directory}\..\Project\QtCreator\build\qctools-gui\release\multimedia\windowsmediaplugin.dll" .
        Pop-Location
        New-Item -ItemType directory -Path "networkinformation"
        Push-Location -Path "networkinformation"
            Copy-Item -Path "${release_directory}\..\Project\QtCreator\build\qctools-gui\release\networkinformation\qnetworklistmanager.dll" .
        Pop-Location
        New-Item -ItemType directory -Path "platforms"
        Push-Location -Path "platforms"
            Copy-Item -Path "${release_directory}\..\Project\QtCreator\build\qctools-gui\release\platforms\qwindows.dll" .
        Pop-Location
        New-Item -ItemType directory -Path "styles"
        Push-Location -Path "styles"
            Copy-Item -Path "${release_directory}\..\Project\QtCreator\build\qctools-gui\release\styles\qwindowsvistastyle.dll" .
        Pop-Location
        New-Item -ItemType directory -Path "tls"
        Push-Location -Path "tls"
            Copy-Item -Path "${release_directory}\..\Project\QtCreator\build\qctools-gui\release\tls\qcertonlybackend.dll" .
            Copy-Item -Path "${release_directory}\..\Project\QtCreator\build\qctools-gui\release\tls\qopensslbackend.dll" .
            Copy-Item -Path "${release_directory}\..\Project\QtCreator\build\qctools-gui\release\tls\qschannelbackend.dll" .
        Pop-Location
        Copy-Item -Path "${release_directory}\..\..\output\lib\qwt.dll" .
        Copy-Item -Path "${release_directory}\..\..\ffmpeg\bin\av*.dll" .
        Copy-Item -Path "${release_directory}\..\..\ffmpeg\bin\sw*.dll" .
        Copy-Item -Path "${release_directory}\..\..\output\bin\freetype-*.dll" .
        Copy-Item -Path "${release_directory}\..\..\output\bin\harfbuzz.dll" .
        Copy-Item -Path "${Env:VCToolsRedistDir}\${arch}\Microsoft.VC143.CRT\concrt140.dll" .
        Copy-Item -Path "${Env:VCToolsRedistDir}\${arch}\Microsoft.VC143.CRT\msvcp140.dll" .
        Copy-Item -Path "${Env:VCToolsRedistDir}\${arch}\Microsoft.VC143.CRT\msvcp140_1.dll" .
        Copy-Item -Path "${Env:VCToolsRedistDir}\${arch}\Microsoft.VC143.CRT\msvcp140_2.dll" .
        Copy-Item -Path "${Env:VCToolsRedistDir}\${arch}\Microsoft.VC143.CRT\msvcp140_atomic_wait.dll" .
        Copy-Item -Path "${Env:VCToolsRedistDir}\${arch}\Microsoft.VC143.CRT\msvcp140_codecvt_ids.dll" .
        Copy-Item -Path "${Env:VCToolsRedistDir}\${arch}\Microsoft.VC143.CRT\vccorlib140.dll" .
        Copy-Item -Path "${Env:VCToolsRedistDir}\${arch}\Microsoft.VC143.CRT\vcruntime140.dll" .
        Copy-Item -Path "${Env:VCToolsRedistDir}\${arch}\Microsoft.VC143.CRT\vcruntime140_1.dll" .
        Copy-Item -Path "${Env:VCToolsRedistDir}\${arch}\Microsoft.VC143.CRT\vcruntime140_threads.dll" .

        Compress-Archive -Path * -DestinationPath "..\QCTools_${version}_Windows_${arch}_WithoutInstaller.zip"
    Pop-Location

    Compress-Archive -Path "${release_directory}\..\Project\QtCreator\build\qctools-gui\release\QCTools.pdb" -DestinationPath "QCTools_${version}_Windows_${arch}_DebugInfo.zip"
Pop-Location

#-----------------------------------------------------------------------
# Package installer
Push-Location "${release_directory}"
    makensis.exe "..\Source\Install\QCTools.nsi"
Pop-Location
