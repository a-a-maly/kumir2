Unicode true
ManifestDPIAware true
!include MUI2.nsh
 
# This include is used only by build system
# For manual build change these lines to set variables OutFile, Name, and InstallDir:
#  OutFile "kumir2-...exe"
#  Name "Кумир-2.x"
#  InstallDir "$PROGRAMFILES\Kumir2x"
#  !define VERSION_SUFFIX "master"
!include nsis_version_info.nsh

RequestExecutionLevel admin

!insertmacro MUI_PAGE_LICENSE "LICENSE_RU.rtf"

!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_LANGUAGE "Russian"


!define SHCNE_ASSOCCHANGED 0x08000000
!define SHCNF_IDLIST 0

Function RefreshShellIcons
  ; By jerome tremblay - april 2003
  System::Call 'shell32.dll::SHChangeNotify(i, i, i, i) v \
  (${SHCNE_ASSOCCHANGED}, ${SHCNF_IDLIST}, 0, 0)'
FunctionEnd

Section "Kumir" Kumir

    SetShellVarContext all

    RMDir /r /REBOOTOK "$INSTDIR\lib\kumir2\plugins" ; Prevent conflicting modules from previous intallation

    SetOutPath "$INSTDIR"
    File LICENSE_RU.rtf
    #File vcredist_x86.exe

    SetOutPath "$INSTDIR\bin"
    File /r bin\*

    SetOutPath "$INSTDIR\lib"
    File /r lib\*

    SetOutPath "$INSTDIR\share"
    File /r share\*

    #SetOutPath "$INSTDIR\src"
    #File /nonfatal /r src\*

    #SetOutPath "$INSTDIR\include"
    #File /r /nonfatal "include\*"

    #SetOutPath "$INSTDIR\llvm-mingw"
    #File /nonfatal /r llvm-mingw\*

    #SetOutPath "$INSTDIR\python"
    #File /nonfatal /r python\*


    #ExecWait '"$INSTDIR\vcredist_x86.exe" /passive'
    #Delete /REBOOTOK "$INSTDIR\vcredist_x86.exe"
    
    WriteRegStr HKCR ".kum" "" "ru.niisi.kumir2.program"
    WriteRegStr HKCR "ru.niisi.kumir2.program" "" "Программа Кумир"
    WriteRegStr HKCR "ru.niisi.kumir2.program\shell\open\command" "" '"$INSTDIR\bin\kumir2-open.exe" "%1"'
    WriteRegStr HKCR "ru.niisi.kumir2.program\shell\Создать выполняемый файл\command" "" 'wscript.exe "$INSTDIR\bin\kumir2-llvmc-w.vbs" "%L"'
    WriteRegStr HKCR "ru.niisi.kumir2.program\DefaultIcon" "" "$INSTDIR\share\icons\kumir2-kum.ico"
    WriteRegStr HKCR ".kod" "" "ru.niisi.kumir2.bytecode"
    WriteRegStr HKCR "ru.niisi.kumir2.bytecode" "" "Выполняемый байткод Кумир"    
    WriteRegStr HKCR "ru.niisi.kumir2.bytecode\shell\open\command" "" '"$INSTDIR\bin\kumir2-run.exe" "%1" "%*"'
    WriteRegStr HKCR "ru.niisi.kumir2.bytecode\DefaultIcon" "" "$INSTDIR\share\icons\kumir2-kod.ico"
    
    Call RefreshShellIcons

    CreateDirectory "$SMPROGRAMS\Кумир ${VERSION_SUFFIX}"
    CreateShortCut "$SMPROGRAMS\Кумир ${VERSION_SUFFIX}\Кумир-Стандарт.lnk" "$INSTDIR\bin\kumir2-classic.exe"
    CreateShortCut "$SMPROGRAMS\Кумир ${VERSION_SUFFIX}\Кумир-Про.lnk" "$INSTDIR\bin\kumir2-ide.exe"
    CreateShortCut "$SMPROGRAMS\Кумир ${VERSION_SUFFIX}\Кумир для учителей.lnk" "$INSTDIR\bin\kumir2-teacher.exe"
    CreateShortCut "$SMPROGRAMS\Кумир ${VERSION_SUFFIX}\Удалить Кумир.lnk" "$INSTDIR\uninstall.exe"
    ; Uninstaller registration
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\kumir_${VERSION_SUFFIX}" "DisplayName" "Кумир ${VERSION_SUFFIX}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\kumir_${VERSION_SUFFIX}" "UninstallString" "$INSTDIR\uninstall.exe"

    WriteUninstaller $INSTDIR\uninstall.exe
    
SectionEnd

function un.onInit
	SetShellVarContext all
 
	#Verify the uninstaller - last chance to back out
        MessageBox MB_OKCANCEL "Удалить Кумир ${VERSION_SUFFIX}?" IDOK next
		Abort
	next:
	
functionEnd

Section "uninstall"

    RMDir /r /REBOOTOK "$INSTDIR\bin"
    RMDir /r /REBOOTOK "$INSTDIR\lib"
    RMDir /r /REBOOTOK "$INSTDIR\share"
    Delete /REBOOTOK "$INSTDIR\LICENSE_RU.rtf"
    Delete /REBOOTOK "$INSTDIR\uninstall.exe"
    RMDir /REBOOTOK "$INSTDIR"

    RMDir /r /REBOOTOK "$SMPROGRAMS\Кумир ${VERSION_SUFFIX}"

    DeleteRegKey HKCR ".kum"
    DeleteRegKey HKCR "ru.niisi.kumir2.program"
    DeleteRegKey HKCR ".kod"
    DeleteRegKey HKCR "ru.niisi.kumir2.bytecode"
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\kumir_${VERSION_SUFFIX}"

SectionEnd
