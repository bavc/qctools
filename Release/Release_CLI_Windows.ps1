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
$artifact = "${release_directory}\QCli_${version}_Windows_${arch}"
if (Test-Path "${artifact}") {
    Remove-Item -Force -Recurse "${artifact}"
}

$artifact = "${release_directory}\QCli_${version}_Windows_${arch}.zip"
if (Test-Path "${artifact}") {
    Remove-Item -Force "${artifact}"
}

$artifact = "${release_directory}\QCli_${version}_Windows_${arch}_DebugInfo.zip"
if (Test-Path "${artifact}") {
    Remove-Item -Force "${artifact}"
}

#-----------------------------------------------------------------------
# Package GUI
Push-Location "${release_directory}"
    New-Item -ItemType Directory -Path "QCli_${version}_Windows_${arch}"
    Push-Location -Path "QCli_${version}_Windows_${arch}"
        Copy-Item -Force -Path "${release_directory}\..\History.txt" .
        Copy-Item -Force -Path "${release_directory}\..\License.html" .
        Copy-Item -Force -Path "${release_directory}\..\Project\QtCreator\build\qctools-cli\release\qcli.exe" .
        Copy-Item -Force -Path "${release_directory}\..\Project\QtCreator\build\qctools-cli\release\Qt6Core.dll" .
        Copy-Item -Force -Path "${release_directory}\..\Project\QtCreator\build\qctools-cli\release\Qt6Gui.dll" .
        Copy-Item -Force -Path "${release_directory}\..\Project\QtCreator\build\qctools-cli\release\Qt6Multimedia.dll" .
        Copy-Item -Force -Path "${release_directory}\..\Project\QtCreator\build\qctools-cli\release\Qt6Network.dll" .
        Copy-Item -Force -Path "${release_directory}\..\Project\QtCreator\build\qctools-cli\release\Qt6Svg.dll" .
        Copy-Item -Force -Path "${release_directory}\..\Project\QtCreator\build\qctools-cli\release\d3dcompiler_47.dll" .
        Copy-Item -Force -Path "${release_directory}\..\Project\QtCreator\build\qctools-cli\release\opengl32sw.dll" .
        New-Item -ItemType directory -Path "iconengines"
        Push-Location -Path "iconengines"
            Copy-Item -Force -Path "${release_directory}\..\Project\QtCreator\build\qctools-cli\release\iconengines\qsvgicon.dll" .
        Pop-Location
        New-Item -ItemType directory -Path "imageformats"
        Push-Location -Path "imageformats"
            Copy-Item -Path "${release_directory}\..\Project\QtCreator\build\qctools-cli\release\imageformats\qjpeg.dll" .
            Copy-Item -Path "${release_directory}\..\Project\QtCreator\build\qctools-cli\release\imageformats\qsvg.dll" .
            Copy-Item -Path "${release_directory}\..\Project\QtCreator\build\qctools-cli\release\imageformats\qico.dll" .
        Pop-Location
        New-Item -ItemType directory -Path "multimedia"
        Push-Location -Path "multimedia"
            Copy-Item -Path "${release_directory}\..\Project\QtCreator\build\qctools-cli\release\multimedia\windowsmediaplugin.dll" .
        Pop-Location
        New-Item -ItemType directory -Path "networkinformation"
        Push-Location -Path "networkinformation"
            Copy-Item -Path "${release_directory}\..\Project\QtCreator\build\qctools-cli\release\networkinformation\qnetworklistmanager.dll" .
        Pop-Location
        New-Item -ItemType directory -Path "platforms"
        Push-Location -Path "platforms"
            Copy-Item -Path "${release_directory}\..\Project\QtCreator\build\qctools-cli\release\platforms\qwindows.dll" .
        Pop-Location
        New-Item -ItemType directory -Name "tls"
        Push-Location -Path "tls"
            Copy-Item -Path "${release_directory}\..\Project\QtCreator\build\qctools-cli\release\tls\qcertonlybackend.dll" .
            Copy-Item -Path "${release_directory}\..\Project\QtCreator\build\qctools-cli\release\tls\qopensslbackend.dll" .
            Copy-Item -Path "${release_directory}\..\Project\QtCreator\build\qctools-cli\release\tls\qschannelbackend.dll" .
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

        Compress-Archive -Path * -DestinationPath "..\QCli_${version}_Windows_${arch}.zip"
    Pop-Location

    Compress-Archive -Path "${release_directory}\..\Project\QtCreator\build\qctools-cli\release\QCli.pdb" -DestinationPath "QCli_${version}_Windows_${arch}_DebugInfo.zip"
Pop-Location
