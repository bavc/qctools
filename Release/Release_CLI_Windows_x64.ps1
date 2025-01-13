###################################################################################################
# Release_CLI_Windows_x64.ps1 - PowerShell script for creating Windows QCli packages (x64)        #
#                                                                                                 #
# Script requirements:                                                                            #
# - Built qctools_AllInclusive sources                                                            #
# - Microsoft Visual Studio environment                                                           #
###################################################################################################

# setup environment
$ErrorActionPreference="Stop"
$build_path=$Pwd
$version=Get-Content -Path $build_path\qctools\Project\version.txt

# binary archive
Write-Output "Create CLI archive"
if (Test-Path -Path QCli_${version}_Windows_x64) {
    Remove-Item -Force -Recurse -Path QCli_${version}_Windows_x64
}
New-Item -ItemType directory -Name QCli_${version}_Windows_x64
Push-Location -Path QCli_${version}_Windows_x64
    Copy-Item -Path $build_path\qctools\History.txt
    Copy-Item -Path $build_path\qctools\License.html
    Copy-Item -Path $build_path\qctools\Project\QtCreator\build\qctools-cli\release\qcli.exe
    Copy-Item -Path $build_path\qctools\Project\QtCreator\build\qctools-cli\release\Qt6Core.dll
    Copy-Item -Path $build_path\qctools\Project\QtCreator\build\qctools-cli\release\Qt6Gui.dll
    Copy-Item -Path $build_path\qctools\Project\QtCreator\build\qctools-cli\release\Qt6Multimedia.dll
    Copy-Item -Path $build_path\qctools\Project\QtCreator\build\qctools-cli\release\Qt6Network.dll
    Copy-Item -Path $build_path\qctools\Project\QtCreator\build\qctools-cli\release\Qt6Svg.dll
    Copy-Item -Path $build_path\qctools\Project\QtCreator\build\qctools-cli\release\d3dcompiler_47.dll
    Copy-Item -Path $build_path\qctools\Project\QtCreator\build\qctools-cli\release\opengl32sw.dll
    New-Item -ItemType directory -Name iconengines
    Push-Location -Path iconengines
        Copy-Item -Path $build_path\qctools\Project\QtCreator\build\qctools-cli\release\iconengines\qsvgicon.dll
    Pop-Location
    New-Item -ItemType directory -Name imageformats
    Push-Location -Path imageformats
        Copy-Item -Path $build_path\qctools\Project\QtCreator\build\qctools-cli\release\imageformats\qjpeg.dll
        Copy-Item -Path $build_path\qctools\Project\QtCreator\build\qctools-cli\release\imageformats\qsvg.dll
        Copy-Item -Path $build_path\qctools\Project\QtCreator\build\qctools-cli\release\imageformats\qico.dll
    Pop-Location
    New-Item -ItemType directory -Name multimedia
    Push-Location -Path multimedia
        Copy-Item -Path $build_path\qctools\Project\QtCreator\build\qctools-cli\release\multimedia\windowsmediaplugin.dll
    Pop-Location
    New-Item -ItemType directory -Name networkinformation
    Push-Location -Path networkinformation
        Copy-Item -Path $build_path\qctools\Project\QtCreator\build\qctools-cli\release\networkinformation\qnetworklistmanager.dll
    Pop-Location
    New-Item -ItemType directory -Name platforms
    Push-Location -Path platforms
        Copy-Item -Path $build_path\qctools\Project\QtCreator\build\qctools-cli\release\platforms\qwindows.dll
    Pop-Location
    New-Item -ItemType directory -Name tls
    Push-Location -Path tls
        Copy-Item -Path $build_path\qctools\Project\QtCreator\build\qctools-cli\release\tls\qcertonlybackend.dll
        Copy-Item -Path $build_path\qctools\Project\QtCreator\build\qctools-cli\release\tls\qopensslbackend.dll
        Copy-Item -Path $build_path\qctools\Project\QtCreator\build\qctools-cli\release\tls\qschannelbackend.dll
    Pop-Location
    Copy-Item -Path $build_path\ffmpeg\bin\avdevice-*.dll
    Copy-Item -Path $build_path\ffmpeg\bin\avcodec-*.dll
    Copy-Item -Path $build_path\ffmpeg\bin\avfilter-*.dll
    Copy-Item -Path $build_path\ffmpeg\bin\avformat-*.dll
    Copy-Item -Path $build_path\ffmpeg\bin\avutil-*.dll
    Copy-Item -Path $build_path\ffmpeg\bin\postproc-*.dll
    Copy-Item -Path $build_path\ffmpeg\bin\swresample-*.dll
    Copy-Item -Path $build_path\ffmpeg\bin\swscale-*.dll
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
if (Test-Path -Path QCli_${version}_Windows_x64.zip) {
    Remove-Item -Force -Path QCli_${version}_Windows_x64.zip
}
Compress-Archive -Path QCli_${version}_Windows_x64\* -DestinationPath QCli_${version}_Windows_x64.zip

# debug symbols archive
Write-Output "Create CLI debug symbols archive"
if (Test-Path -Path QCli_${version}_Windows_x64_DebugInfo.zip) {
    Remove-Item -Force -Path QCli_${version}_Windows_x64_DebugInfo.zip
}
Compress-Archive -Path $build_path\qctools\Project\QtCreator\build\qctools-cli\release\QCli.pdb -DestinationPath QCli_${version}_Windows_x64_DebugInfo.zip
