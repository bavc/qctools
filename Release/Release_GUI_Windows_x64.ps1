###################################################################################################
# Release_GUI_Windows_x64.ps1 - PowerShell script for creating Windows QCTools packages (x64)     #
#                                                                                                 #
# Script requirements:                                                                            #
# - Built qctools_AllInclusive sources                                                            #
# - Microsoft Visual Studio environment                                                           #
# - NSIS binary in the PATH                                                                       #
###################################################################################################

# helpers
function Cmd-Result {
    if (-Not $?) {
        Exit(1)
    }
}

# setup environment
$ErrorActionPreference="Stop"
$build_path=$Pwd
$version=Get-Content -Path $build_path\qctools\Project\version.txt

# binary archive
Write-Output "Create GUI archive"
if (Test-Path -Path QCTools_${version}_Windows_x64) {
    Remove-Item -Force -Recurse -Path QCTools_${version}_Windows_x64
}
New-Item -ItemType directory -Name QCTools_${version}_Windows_x64
Push-Location -Path QCTools_${version}_Windows_x64
    Copy-Item -Path $build_path\qctools\History.txt
    Copy-Item -Path $build_path\qctools\License.html
    Copy-Item -Path $build_path\qctools\Project\QtCreator\build\qctools-gui\release\QCTools.exe
    Copy-Item -Path $build_path\qctools\Project\QtCreator\build\qctools-gui\release\Qt6Core.dll
    Copy-Item -Path $build_path\qctools\Project\QtCreator\build\qctools-gui\release\Qt6Gui.dll
    Copy-Item -Path $build_path\qctools\Project\QtCreator\build\qctools-gui\release\Qt6Multimedia.dll
    Copy-Item -Path $build_path\qctools\Project\QtCreator\build\qctools-gui\release\Qt6MultimediaWidgets.dll
    Copy-Item -Path $build_path\qctools\Project\QtCreator\build\qctools-gui\release\Qt6Network.dll
    Copy-Item -Path $build_path\qctools\Project\QtCreator\build\qctools-gui\release\Qt6OpenGL.dll
    Copy-Item -Path $build_path\qctools\Project\QtCreator\build\qctools-gui\release\Qt6Qml.dll
    Copy-Item -Path $build_path\qctools\Project\QtCreator\build\qctools-gui\release\Qt6QmlModels.dll
    Copy-Item -Path $build_path\qctools\Project\QtCreator\build\qctools-gui\release\Qt6Svg.dll
    Copy-Item -Path $build_path\qctools\Project\QtCreator\build\qctools-gui\release\Qt6Widgets.dll
    Copy-Item -Path $build_path\qctools\Project\QtCreator\build\qctools-gui\release\d3dcompiler_47.dll
    Copy-Item -Path $build_path\qctools\Project\QtCreator\build\qctools-gui\release\opengl32sw.dll
    New-Item -ItemType directory -Name iconengines
    Push-Location -Path iconengines
        Copy-Item -Path $build_path\qctools\Project\QtCreator\build\qctools-gui\release\iconengines\qsvgicon.dll
    Pop-Location
    New-Item -ItemType directory -Name imageformats
    Push-Location -Path imageformats
        Copy-Item -Path $build_path\qctools\Project\QtCreator\build\qctools-gui\release\imageformats\qjpeg.dll
        Copy-Item -Path $build_path\qctools\Project\QtCreator\build\qctools-gui\release\imageformats\qsvg.dll
        Copy-Item -Path $build_path\qctools\Project\QtCreator\build\qctools-gui\release\imageformats\qico.dll
    Pop-Location
    New-Item -ItemType directory -Name multimedia
    Push-Location -Path multimedia
        Copy-Item -Path $build_path\qctools\Project\QtCreator\build\qctools-gui\release\multimedia\windowsmediaplugin.dll
    Pop-Location
    New-Item -ItemType directory -Name networkinformation
    Push-Location -Path networkinformation
        Copy-Item -Path $build_path\qctools\Project\QtCreator\build\qctools-gui\release\networkinformation\qnetworklistmanager.dll
    Pop-Location
    New-Item -ItemType directory -Name platforms
    Push-Location -Path platforms
        Copy-Item -Path $build_path\qctools\Project\QtCreator\build\qctools-gui\release\platforms\qwindows.dll
    Pop-Location
    New-Item -ItemType directory -Name styles
    Push-Location -Path styles
        Copy-Item -Path $build_path\qctools\Project\QtCreator\build\qctools-gui\release\styles\qwindowsvistastyle.dll
    Pop-Location
    New-Item -ItemType directory -Name tls
    Push-Location -Path tls
        Copy-Item -Path $build_path\qctools\Project\QtCreator\build\qctools-gui\release\tls\qcertonlybackend.dll
        Copy-Item -Path $build_path\qctools\Project\QtCreator\build\qctools-gui\release\tls\qopensslbackend.dll
        Copy-Item -Path $build_path\qctools\Project\QtCreator\build\qctools-gui\release\tls\qschannelbackend.dll
    Pop-Location
    Copy-Item -Path $build_path\output\lib\qwt.dll
    Copy-Item -Path $build_path\output\bin\avdevice-*.dll
    Copy-Item -Path $build_path\output\bin\avcodec-*.dll
    Copy-Item -Path $build_path\output\bin\avfilter-*.dll
    Copy-Item -Path $build_path\output\bin\avformat-*.dll
    Copy-Item -Path $build_path\output\bin\avutil-*.dll
    Copy-Item -Path $build_path\output\bin\postproc-*.dll
    Copy-Item -Path $build_path\output\bin\swresample-*.dll
    Copy-Item -Path $build_path\output\bin\swscale-*.dll
    Copy-Item -Path $build_path\output\bin\freetype-*.dll
    Copy-Item -Path $build_path\output\bin\harfbuzz.dll
    Copy-Item -Path $Env:VCToolsRedistDir\x64\Microsoft.VC143.CRT\concrt140.dll
    Copy-Item -Path $Env:VCToolsRedistDir\x64\Microsoft.VC143.CRT\msvcp140.dll
    Copy-Item -Path $Env:VCToolsRedistDir\x64\Microsoft.VC143.CRT\msvcp140_1.dll
    Copy-Item -Path $Env:VCToolsRedistDir\x64\Microsoft.VC143.CRT\msvcp140_2.dll
    Copy-Item -Path $Env:VCToolsRedistDir\x64\Microsoft.VC143.CRT\msvcp140_atomic_wait.dll
    Copy-Item -Path $Env:VCToolsRedistDir\x64\Microsoft.VC143.CRT\msvcp140_codecvt_ids.dll
    Copy-Item -Path $Env:VCToolsRedistDir\x64\Microsoft.VC143.CRT\vccorlib140.dll
    Copy-Item -Path $Env:VCToolsRedistDir\x64\Microsoft.VC143.CRT\vcruntime140.dll
    Copy-Item -Path $Env:VCToolsRedistDir\x64\Microsoft.VC143.CRT\vcruntime140_1.dll
    Copy-Item -Path $Env:VCToolsRedistDir\x64\Microsoft.VC143.CRT\vcruntime140_threads.dll
Pop-Location
if (Test-Path -Path QCTools_${version}_Windows_x64_WithoutInstaller.zip) {
    Remove-Item -Force -Path QCTools_${version}_Windows_x64_WithoutInstaller.zip
}
Compress-Archive -Path QCTools_${version}_Windows_x64\* -DestinationPath QCTools_${version}_Windows_x64_WithoutInstaller.zip

# debug symbols archive
Write-Output "Create GUI debug symbols archive"
if (Test-Path -Path QCTools_${version}_Windows_x64_DebugInfo.zip) {
    Remove-Item -Force -Path QCTools_${version}_Windows_x64_DebugInfo.zip
}
Compress-Archive -Path $build_path\qctools\Project\QtCreator\build\qctools-gui\release\QCTools.pdb -DestinationPath QCTools_${version}_Windows_x64_DebugInfo.zip

# installer
Write-Output "Create GUI installer"
if (Test-Path -Path QCTools_${version}_Windows.exe) {
    Remove-Item -Force -Path QCTools_${version}_Windows.exe
}
Push-Location -Path qctools\Source\Install
    makensis.exe QCTools.nsi ; Cmd-Result
Pop-Location
Copy-Item -Path qctools\QCTools_${version}_Windows.exe
