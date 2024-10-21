!include "MUI2.nsh"

Name "KholorsStation"

Var VstInstDir

!define APP_NAME "KholorsStation"
!define APP_VERSION "1.0.0"
!define APP_DIR "$PROGRAMFILES64\ArtifaktNd\${APP_NAME}"
!define APP_ICON "kholors.ico"

!define MUI_FINISHPAGE_TEXT "Thank you for installing KholorsStation and KholorsSink. You will be asked for the License key when starting the KholorsStation app (available in the startup menu). You can open the app and click the Help button to get access to the online documentation and tutorials. Have fun using Kholors!"

!define MUI_ICON "kholors.ico"
!define MUI_UNICON "kholors.ico"

!define MUI_LICENSE_PAGE_TITLE "End User License Agreement"
!define MUI_LICENSE_PAGE_TEXT "Please read the following license agreement carefully."
!define MUI_LICENSE_PAGE_CUSTOMTEXT "This license applies to both the KholorsStation and the KholorsSink VST3 plugin software."
!define MUI_TEXT_LICENSE_TITLE "End User License Agreement"

Outfile "${APP_NAME}-${APP_VERSION}-Install.exe"
InstallDir "${APP_DIR}"

RequestExecutionLevel admin

Section "MainSection" SEC01
   SetOutPath "$INSTDIR"
   File "..\build\src\StationApp\StationApp_artefacts\Debug\KholorsStation.exe"
   File "kholors.ico"

   SetOutPath "$VstInstDir\KholorsSink.vst3"
   File /nonfatal /a /r "..\build\src\SinkPlugin\SinkPlugin_artefacts\Debug\VST3\KholorsSink.vst3\" #note back slash at the end

   CreateShortCut "$SMSTARTUP\KholorsStation.lnk" "$INSTDIR\KholorsStation.exe" "" "$INSTDIR\KholorsStation.exe,0"
   CreateShortCut "$SMPROGRAMS\ArtifaktNd\KholorsStation.lnk" "$INSTDIR\KholorsStation.exe" "" "$INSTDIR\kholors.ico" 0

   WriteUninstaller "$INSTDIR\${APP_NAME}-${APP_VERSION}-Uninstall.exe"
SectionEnd

Section "Uninstall"
   Delete "$INSTDIR\KholorsStation.exe"
   Delete "$INSTDIR\kholors.ico"
   Delete "$SMPROGRAMS\ArtifaktNd\KholorsStation.lnk"
   Delete "$SMSTARTUP\KholorsStation.lnk"
   Delete "$INSTDIR\${APP_NAME}-${APP_VERSION}-Uninstall.exe"
   RMDir /r "$VstInstDir\KholorsSink.vst3"
   RMDir "$INSTDIR"
SectionEnd

Function .onInit
   SetRegView 64
   CreateDirectory "$SMPROGRAMS\ArtifaktNd"
   StrCpy $VstInstDir "$PROGRAMFILES64\Common Files\VST3"
FunctionEnd

Function un.onInit
   SetRegView 64
   StrCpy $VstInstDir "$PROGRAMFILES64\Common Files\VST3"
FunctionEnd

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "EULA.txt"
!insertmacro MUI_PAGE_DIRECTORY
!define MUI_PAGE_HEADER_TEXT "Steinberg VST3 plugin location"
!define MUI_PAGE_HEADER_SUBTEXT "Choose the folder in which to install the KholorsSink VST3 plugin."
!define MUI_DIRECTORYPAGE_TEXT_TOP "The installer will install the VST3 plugin for the KholorsStation in the following folder. To install in a differenct folder, click Browse and select another folder. Click Next to continue."
!define MUI_DIRECTORYPAGE_VARIABLE $VstInstDir
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_WELCOME
!define MUI_PAGE_HEADER_TEXT "Steinberg VST3 plugin location"
!define MUI_PAGE_HEADER_SUBTEXT "Choose the folder the KholorsSink VST3 plugin was installed to."
!define MUI_DIRECTORYPAGE_TEXT_TOP "The uninstaller will remove the VST3 plugin from the following folder. Click Next to continue."
!define MUI_DIRECTORYPAGE_VARIABLE $VstInstDir
!insertmacro MUI_UNPAGE_DIRECTORY

!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH
!insertmacro MUI_LANGUAGE "English"
