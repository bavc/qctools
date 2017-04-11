#NSIS: encoding=UTF-8
; Request application privileges for Windows Vista
RequestExecutionLevel admin

; Some defines
!define PRODUCT_NAME "QCTools"
!define PRODUCT_PUBLISHER "MediaArea.net"
!define PRODUCT_VERSION "0.8.0"
!define PRODUCT_VERSION4 "${PRODUCT_VERSION}.0"
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
  ${EndIf}
FunctionEnd

Section "SectionPrincipale" SEC01
  SetOverwrite on
  SetOutPath "$SMPROGRAMS"
  CreateShortCut "$SMPROGRAMS\QCTools.lnk" "$INSTDIR\QCTools.exe" "" "$INSTDIR\QCTools.exe" 0 "" "" "QCTools"
  SetOutPath "$INSTDIR"
  !ifndef STATIC
    File "..\..\Project\MSVC2015\Release\QCTools.exe"
  !else
    File "..\..\Project\MSVC2015\StaticRelease\QCTools.exe"
  !endif
  File "..\..\History.txt"
  File "..\..\License.html"
  !ifndef STATIC
    File "..\..\..\ffmpeg\libavcodec\avcodec-*.dll"
    File "..\..\..\ffmpeg\libavdevice\avdevice-*.dll"
    File "..\..\..\ffmpeg\libavfilter\avfilter-*.dll"
    File "..\..\..\ffmpeg\libavformat\avformat-*.dll"
    File "..\..\..\ffmpeg\libavutil\avutil-*.dll"
    File "..\..\..\ffmpeg\libpostproc\postproc-*.dll"
    File "..\..\..\ffmpeg\libswresample\swresample-*.dll"
    File "..\..\..\ffmpeg\libswscale\swscale-*.dll"
    File "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\redist\x86\Microsoft.VC140.CRT\concrt140.dll"
    File "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\redist\x86\Microsoft.VC140.CRT\msvcp140.dll"
    File "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\redist\x86\Microsoft.VC140.CRT\vccorlib140.dll"
    File "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\redist\x86\Microsoft.VC140.CRT\vcruntime140.dll"
    File "..\..\..\Qt\bin\Qt5Core.dll"
    File "..\..\..\Qt\bin\Qt5Gui.dll"
    File "..\..\..\Qt\bin\Qt5Widgets.dll"
    SetOutPath "$INSTDIR\imageformats"
    File "..\..\..\Qt\plugins\imageformats\qjpeg.dll"
    SetOutPath "$INSTDIR\platforms"
    File "..\..\..\Qt\plugins\platforms\qwindows.dll"
  !endif
  # Create files
  WriteIniStr "$INSTDIR\${PRODUCT_NAME}.url" "InternetShortcut" "URL" "${PRODUCT_WEB_SITE}"
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
  Delete "$INSTDIR\History.txt"
  Delete "$INSTDIR\License.html"
  !ifndef STATIC
    Delete "$INSTDIR\avcodec-*.dll"
    Delete "$INSTDIR\avdevice-*.dll"
    Delete "$INSTDIR\avfilter-*.dll"
    Delete "$INSTDIR\avformat-*.dll"
    Delete "$INSTDIR\avutil-*.dll"
    Delete "$INSTDIR\postproc-*.dll"
    Delete "$INSTDIR\swresample-*.dll"
    Delete "$INSTDIR\swscale-*.dll"
    Delete "$INSTDIR\imageformats\qjpeg.dll"
    Delete "$INSTDIR\platforms\qwindows.dll"
    Delete "$INSTDIR\Qt5Core.dll"
    Delete "$INSTDIR\Qt5Gui.dll"
    Delete "$INSTDIR\Qt5Widgets.dll"
    Delete "$INSTDIR\concrt140.dll"
    Delete "$INSTDIR\msvcp140.dll"
    Delete "$INSTDIR\vccorlib140.dll"
    Delete "$INSTDIR\vcruntime140.dll"
    RMDir "$INSTDIR\imageformats"
    RMDir "$INSTDIR\platforms"
  !endif
  RMDir  "$INSTDIR"
  Delete "$SMPROGRAMS\QCTools.lnk"

  SetRegView 64
  DeleteRegKey HKLM "${PRODUCT_REGISTRY}"
  DeleteRegKey /ifempty HKLM "${COMPANY_REGISTRY}"
  DeleteRegKey HKCU "${PRODUCT_REGISTRY}"
  DeleteRegKey /ifempty HKCU "${COMPANY_REGISTRY}"
  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
  SetAutoClose true
SectionEnd
