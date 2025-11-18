#NSIS: encoding=UTF-8
; Request application privileges for Windows Vista
RequestExecutionLevel admin

; Some defines
!define PRODUCT_NAME "QCTools"
!define PRODUCT_PUBLISHER "MediaArea.net"
!define PRODUCT_VERSION "1.4"
!define PRODUCT_VERSION4 "${PRODUCT_VERSION}.0.0"
!define PRODUCT_WEB_SITE "http://www.bavc.org/qctools"
!define COMPANY_REGISTRY "Software\MediaArea.net"
!define PRODUCT_REGISTRY "Software\MediaArea.net\QCTools"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\QCTools.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

; Compression
SetCompressor /FINAL /SOLID lzma

; Conditional
!include LogicLib.nsh

; x64 stuff
!include "x64.nsh"

; File size
!include FileFunc.nsh
!include WinVer.nsh

; Modern UI
!include "MUI2.nsh"
!define MUI_ABORTWARNING
!define MUI_ICON "..\..\Source\Resource\Logo.ico"

; Language Selection Dialog Settings
!define MUI_LANGDLL_REGISTRY_ROOT "${PRODUCT_UNINST_ROOT_KEY}"
!define MUI_LANGDLL_REGISTRY_KEY "${PRODUCT_UNINST_KEY}"
!define MUI_LANGDLL_REGISTRY_VALUENAME "NSIS:Language"

; Installer pages
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
; !define MUI_FINISHPAGE_RUN "$INSTDIR\QCTools.exe" //Removing it because it is run in admin privileges
!insertmacro MUI_PAGE_FINISH
; Uninstaller pages
!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

; Language files
!insertmacro MUI_LANGUAGE "English"

; Info
VIProductVersion "${PRODUCT_VERSION4}"
VIAddVersionKey "CompanyName"      "${PRODUCT_PUBLISHER}"
VIAddVersionKey "ProductName"      "${PRODUCT_NAME}"
VIAddVersionKey "ProductVersion"   "${PRODUCT_VERSION4}"
VIAddVersionKey "FileDescription"  "QCTools"
VIAddVersionKey "FileVersion"      "${PRODUCT_VERSION4}"
VIAddVersionKey "LegalCopyright"   "${PRODUCT_PUBLISHER}"
VIAddVersionKey "OriginalFilename" "${PRODUCT_NAME}_${PRODUCT_VERSION}_Windows.exe"
BrandingText " "

; Modern UI end

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "..\..\${PRODUCT_NAME}_${PRODUCT_VERSION}_Windows.exe"
InstallDir "$PROGRAMFILES64\${PRODUCT_NAME}"
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails nevershow
ShowUnInstDetails nevershow

Function .onInit
  ${If} ${RunningX64}
    SetRegView 64
  ${Else}
    MessageBox MB_OK|MB_ICONSTOP 'You are trying to install the 64-bit version of ${PRODUCT_NAME} on 32-bit Windows.$\r$\nPlease download and use the 32-bit version instead.$\r$\nClick OK to quit Setup.'
    Quit
  ${EndIf}
FunctionEnd

Section "SectionPrincipale" SEC01
  SetOverwrite on
  SetOutPath "$SMPROGRAMS"
  CreateShortCut "$SMPROGRAMS\QCTools.lnk" "$INSTDIR\QCTools.exe" "" "$INSTDIR\QCTools.exe" 0 "" "" "QCTools"
  SetOutPath "$INSTDIR"
  !ifndef STATIC
    File "..\..\Project\QtCreator\build\qctools-gui\release\QCTools.exe"
    File "..\..\Project\QtCreator\build\qctools-cli\release\qcli.exe"
  !else
    File "..\..\Project\QtCreator\build\qctools-gui\StaticRelease\QCTools.exe"
    File "..\..\Project\QtCreator\build\qctools-cli\StaticRelease\qcli.exe"
  !endif
  File "..\..\History.txt"
  File "..\..\License.html"
  !ifndef STATIC
    File "..\..\..\output\lib\qwt.dll"
    File "..\..\..\output\bin\avdevice-*.dll"
    File "..\..\..\output\bin\avcodec-*.dll"
    File "..\..\..\output\bin\avfilter-*.dll"
    File "..\..\..\output\bin\avformat-*.dll"
    File "..\..\..\output\bin\avutil-*.dll"
    File "..\..\..\output\bin\postproc-*.dll"
    File "..\..\..\output\bin\swresample-*.dll"
    File "..\..\..\output\bin\swscale-*.dll"
    File "..\..\..\output\bin\freetype-*.dll"
    File "..\..\..\output\bin\harfbuzz.dll"
    File "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.44.35112\X64\Microsoft.VC143.CRT\concrt140.dll"
    File "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.44.35112\X64\Microsoft.VC143.CRT\msvcp140.dll"
    File "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.44.35112\X64\Microsoft.VC143.CRT\msvcp140_1.dll"
    File "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.44.35112\X64\Microsoft.VC143.CRT\msvcp140_2.dll"
    File "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.44.35112\X64\Microsoft.VC143.CRT\msvcp140_atomic_wait.dll"
    File "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.44.35112\X64\Microsoft.VC143.CRT\msvcp140_codecvt_ids.dll"
    File "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.44.35112\X64\Microsoft.VC143.CRT\vccorlib140.dll"
    File "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.44.35112\X64\Microsoft.VC143.CRT\vcruntime140.dll"
    File "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.44.35112\X64\Microsoft.VC143.CRT\vcruntime140_1.dll"
    File "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.44.35112\X64\Microsoft.VC143.CRT\vcruntime140_threads.dll"
    File "..\..\Project\QtCreator\build\qctools-gui\release\Qt6Core.dll"
    File "..\..\Project\QtCreator\build\qctools-gui\release\Qt6Gui.dll"
    File "..\..\Project\QtCreator\build\qctools-gui\release\Qt6Multimedia.dll"
    File "..\..\Project\QtCreator\build\qctools-gui\release\Qt6MultimediaWidgets.dll"
    File "..\..\Project\QtCreator\build\qctools-gui\release\Qt6Network.dll"
    File "..\..\Project\QtCreator\build\qctools-gui\release\Qt6OpenGL.dll"
    File "..\..\Project\QtCreator\build\qctools-gui\release\Qt6Qml.dll"
    File "..\..\Project\QtCreator\build\qctools-gui\release\Qt6QmlModels.dll"
    File "..\..\Project\QtCreator\build\qctools-gui\release\Qt6Svg.dll"
    File "..\..\Project\QtCreator\build\qctools-gui\release\Qt6Widgets.dll"
    File "..\..\Project\QtCreator\build\qctools-gui\release\d3dcompiler_47.dll"
    File "..\..\Project\QtCreator\build\qctools-gui\release\opengl32sw.dll"
    SetOutPath "$INSTDIR\iconengines"
    File "..\..\Project\QtCreator\build\qctools-gui\release\iconengines\qsvgicon.dll"
    SetOutPath "$INSTDIR\imageformats"
    File "..\..\Project\QtCreator\build\qctools-gui\release\imageformats\qjpeg.dll"
    File "..\..\Project\QtCreator\build\qctools-gui\release\imageformats\qsvg.dll"
    File "..\..\Project\QtCreator\build\qctools-gui\release\imageformats\qico.dll"
    SetOutPath "$INSTDIR\multimedia"
    File "..\..\Project\QtCreator\build\qctools-gui\release\multimedia\windowsmediaplugin.dll"
    SetOutPath "$INSTDIR\networkinformation"
    File "..\..\Project\QtCreator\build\qctools-gui\release\networkinformation\qnetworklistmanager.dll"
    SetOutPath "$INSTDIR\platforms"
    File "..\..\Project\QtCreator\build\qctools-gui\release\platforms\qwindows.dll"
    SetOutPath "$INSTDIR\styles"
    File "..\..\Project\QtCreator\build\qctools-gui\release\styles\qwindowsvistastyle.dll"
    SetOutPath "$INSTDIR\tls"
    File "..\..\Project\QtCreator\build\qctools-gui\release\tls\qcertonlybackend.dll"
    File "..\..\Project\QtCreator\build\qctools-gui\release\tls\qopensslbackend.dll"
    File "..\..\Project\QtCreator\build\qctools-gui\release\tls\qschannelbackend.dll"

  !endif
  # Create files
  WriteIniStr "$INSTDIR\${PRODUCT_NAME}.url" "InternetShortcut" "URL" "${PRODUCT_WEB_SITE}"

  # Delete files that might be present from older installation
  Delete "$INSTDIR\freetype.dll"
SectionEnd

Section -Post
  WriteUninstaller "$INSTDIR\uninst.exe"
  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\QCTools.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName"     "$(^Name)"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher"       "${PRODUCT_PUBLISHER}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon"     "$INSTDIR\QCTools.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion"  "${PRODUCT_VERSION}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout"    "${PRODUCT_WEB_SITE}"

  ${If} ${AtLeastWin7}
    ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
    IntFmt $0 "0x%08X" $0 ; Convert the decimal KB value in $0 to DWORD, put it right back into $0
    WriteRegDWORD ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "EstimatedSize" "$0" ; Create/Write the reg key with the dword value
  ${EndIf}
SectionEnd


Section Uninstall
  SetRegView 64

  Delete "$INSTDIR\${PRODUCT_NAME}.url"
  Delete "$INSTDIR\uninst.exe"
  Delete "$INSTDIR\QCTools.exe"
  Delete "$INSTDIR\qcli.exe"
  Delete "$INSTDIR\History.txt"
  Delete "$INSTDIR\License.html"
  Delete "$INSTDIR\avdevice-*.dll"
  Delete "$INSTDIR\avcodec-*.dll"
  Delete "$INSTDIR\avfilter-*.dll"
  Delete "$INSTDIR\avformat-*.dll"
  Delete "$INSTDIR\avutil-*.dll"
  Delete "$INSTDIR\postproc-*.dll"
  Delete "$INSTDIR\swresample-*.dll"
  Delete "$INSTDIR\swscale-*.dll"
  Delete "$INSTDIR\freetype-*.dll"
  Delete "$INSTDIR\harfbuzz.dll"
  Delete "$INSTDIR\concrt140.dll"
  Delete "$INSTDIR\msvcp140.dll"
  Delete "$INSTDIR\msvcp140_1.dll"
  Delete "$INSTDIR\msvcp140_2.dll"
  Delete "$INSTDIR\msvcp140_atomic_wait.dll"
  Delete "$INSTDIR\msvcp140_codecvt_ids.dll"
  Delete "$INSTDIR\vccorlib140.dll"
  Delete "$INSTDIR\vcruntime140.dll"
  Delete "$INSTDIR\vcruntime140_1.dll"
  Delete "$INSTDIR\vcruntime140_threads.dll"
  Delete "$INSTDIR\qwt.dll"
  Delete "$INSTDIR\Qt6Core.dll"
  Delete "$INSTDIR\Qt6Gui.dll"
  Delete "$INSTDIR\Qt6Multimedia.dll"
  Delete "$INSTDIR\Qt6MultimediaWidgets.dll"
  Delete "$INSTDIR\Qt6Network.dll"
  Delete "$INSTDIR\Qt6OpenGL.dll"
  Delete "$INSTDIR\Qt6Qml.dll"
  Delete "$INSTDIR\Qt6QmlModels.dll"
  Delete "$INSTDIR\Qt6Svg.dll"
  Delete "$INSTDIR\Qt6Widgets.dll"
  Delete "$INSTDIR\d3dcompiler_47.dll"
  Delete "$INSTDIR\opengl32sw.dll"
  Delete "$INSTDIR\iconengines\qsvgicon.dll"
  Delete "$INSTDIR\imageformats\qjpeg.dll"
  Delete "$INSTDIR\imageformats\qsvg.dll"
  Delete "$INSTDIR\imageformats\qico.dll"
  Delete "$INSTDIR\multimedia\windowsmediaplugin.dll"
  Delete "$INSTDIR\networkinformation\qnetworklistmanager.dll"
  Delete "$INSTDIR\platforms\qwindows.dll"
  Delete "$INSTDIR\styles\qwindowsvistastyle.dll"
  Delete "$INSTDIR\tls\qcertonlybackend.dll"
  Delete "$INSTDIR\tls\qopensslbackend.dll"
  Delete "$INSTDIR\tls\qschannelbackend.dll"
  RMDir "$INSTDIR\iconengines"
  RMDir "$INSTDIR\imageformats"
  RMDir "$INSTDIR\multimedia"
  RMDir "$INSTDIR\networkinformation"
  RMDir "$INSTDIR\platforms"
  RMDir "$INSTDIR\styles"
  RMDir "$INSTDIR\tls"
  RMDir  "$INSTDIR"
  Delete "$SMPROGRAMS\QCTools.lnk"
  ; Olds
  Delete "$INSTDIR\QtAVPlayer.dll"
  Delete "$INSTDIR\Qt5Core.dll"
  Delete "$INSTDIR\Qt5Gui.dll"
  Delete "$INSTDIR\Qt5Network.dll"
  Delete "$INSTDIR\Qt5Multimedia.dll"
  Delete "$INSTDIR\Qt5MultimediaWidgets.dll"
  Delete "$INSTDIR\Qt5Qml.dll"
  Delete "$INSTDIR\Qt5Svg.dll"
  Delete "$INSTDIR\Qt5Widgets.dll"
  Delete "$INSTDIR\d3dcompiler_47.dll"
  Delete "$INSTDIR\libEGL.dll"
  Delete "$INSTDIR\libGLESV2.dll"
  Delete "$INSTDIR\audio\qtaudio_wasapi.dll"
  Delete "$INSTDIR\audio\qtaudio_windows.dll"
  Delete "$INSTDIR\bearer\qgenericbearer.dll"
  Delete "$INSTDIR\mediaservice\dsengine.dll"
  Delete "$INSTDIR\mediaservice\wmfengine.dll"
  Delete "$INSTDIR\mediaservice\qtmedia_audioengine.dll"
  RMDir "$INSTDIR\audio"
  RMDir "$INSTDIR\bearer"
  RMDir "$INSTDIR\mediaservice"

  SetRegView 64
  DeleteRegKey HKLM "${PRODUCT_REGISTRY}"
  DeleteRegKey /ifempty HKLM "${COMPANY_REGISTRY}"
  DeleteRegKey HKCU "${PRODUCT_REGISTRY}"
  DeleteRegKey /ifempty HKCU "${COMPANY_REGISTRY}"
  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
  SetAutoClose true
SectionEnd
