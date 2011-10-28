@ECHO OFF
SETLOCAL ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION

ECHO Executing %~n0 tasks..

REM NOTE:
REM - _BUILDARCH = x86, amd64, or ia64
REM - G_TARGET_OUTPUT_ARCH = i386,  amd64, or ia64
REM

IF /I "!G_DIST!" NEQ "finalize" (
	IF /I "!_BUILDARCH!" EQU "x86" CALL :DistFixup
	goto Done
)

IF EXIST "!K_LIBUSB10_DEP_DIR!" (
	PUSHD !CD!
	CD /D "!K_LIBUSB10_DEP_DIR!\msvc"
	CALL ddk_build
	IF NOT EXIST "!K_LIBUSB10_OUTPUT_DIR!\!K_LIBUSB10_NAME!.lib" (
		POPD
		goto Error
	)
	COPY /Y "!K_LIBUSB10_OUTPUT_DIR!\!K_LIBUSB10_NAME!.lib" "!G_BUILD_OUTPUT_BASE_ABS_DIR!\lib\!G_TARGET_OUTPUT_ARCH!"
	
	CALL ddk_build DLL
	IF NOT EXIST "!K_LIBUSB10_OUTPUT_DIR!\!K_LIBUSB10_NAME!.dll" (
		POPD
		goto Error
	)
	POPD
	
	COPY /Y "!K_LIBUSB10_OUTPUT_DIR!\!K_LIBUSB10_NAME!.dll" "!G_TARGET_OUTPUT_ABS_DIR!\!G_TARGET_OUTPUT_ARCH!\"
	COPY /Y "!K_LIBUSB10_OUTPUT_DIR!\!K_LIBUSB10_NAME!.lib" "!G_TARGET_OUTPUT_ABS_DIR!\!G_TARGET_OUTPUT_ARCH!\"
)

IF EXIST "!K_LIBUSB0_DEP_DIR!" (
	IF /I "!_BUILDARCH!" EQU "x86" (
		COPY /Y "!K_LIBUSB0_DEP_DIR!\bin\!_BUILDARCH!\libusb0_x86.dll" "!G_TARGET_OUTPUT_ABS_DIR!\!G_TARGET_OUTPUT_ARCH!\libusb0.dll"
		COPY /Y "!K_LIBUSB0_DEP_DIR!\lib\msvc\libusb.lib" "!G_TARGET_OUTPUT_ABS_DIR!\!G_TARGET_OUTPUT_ARCH!\libusb0.lib"
	) ELSE (
		COPY /Y "!K_LIBUSB0_DEP_DIR!\bin\!_BUILDARCH!\libusb0.dll" "!G_TARGET_OUTPUT_ABS_DIR!\!G_TARGET_OUTPUT_ARCH!\"
		IF /I "!_BUILDARCH!" EQU "ia64" (
			COPY /Y "!K_LIBUSB0_DEP_DIR!\lib\msvc_i64\libusb.lib" "!G_TARGET_OUTPUT_ABS_DIR!\!G_TARGET_OUTPUT_ARCH!\libusb0.lib"
		) ELSE (
			COPY /Y "!K_LIBUSB0_DEP_DIR!\lib\msvc_x64\libusb.lib" "!G_TARGET_OUTPUT_ABS_DIR!\!G_TARGET_OUTPUT_ARCH!\libusb0.lib"
		)
	)
)

IF /I "!_BUILDARCH!" EQU "x86" (
	REM - make.cfg is configured so the x86 dll is the very last to build.
	REM - This batch file is ran while inside of the x86 ddk environment,
	REM - thus it is used to build libwdi and inf-wizard was well.
	CALL :FinalizeDistribution
)
GOTO Done

:Error
	ECHO Cannot create final dist. Missing !K_LIBUSB10_NAME!/libusb-win32.
	ECHO Update %0 with the correct directories.
	GOTO Done

:Done
GOTO :EOF

:DistFixup
	REM - Get rid of those i386 dirs.
	CALL :RenameOutputSubDirs "\sys\i386" "\sys\x86" "\lib\i386" "\lib\x86" "\dll\i386" "\dll\x86" "\exe\i386" "\exe\x86"
	
	REM - The lib dir contains the static libs; move these into a static subdir.
	CALL :MoveOutputFiles "\lib\x86\*.lib" "\lib\static\x86" "\lib\amd64\*.lib" "\lib\static\amd64" "\lib\ia64\*.lib" "\lib\static\ia64"
	
	REM - Move the dynamic .lib files to the lib dir.
	CALL :MoveOutputFiles "\dll\x86\*.lib" "\lib\x86" "\dll\amd64\*.lib" "\lib\amd64" "\dll\ia64\*.lib" "\lib\ia64"
GOTO :EOF

:FinalizeDistribution
	ECHO [FinalizeDistribution]
	ECHO.
	
	SET K_PKG=!G_PACKAGE_TEMP_ABS_DIR!
	SET K_PKG_BIN=!K_PKG!\!G_BUILD_OUTPUT_BASE_DIR!
	
	REM - Remove pdb and exp files; leave the driver pdb.
	DEL /S /Q !G_BUILD_OUTPUT_BASE_ABS_DIR!\dll\*.pdb
	DEL /S /Q !G_BUILD_OUTPUT_BASE_ABS_DIR!\*.exp

	CALL :DistFixup
	
	REM - Copy the entire binary tree to K_PKG_BIN
	XCOPY "!G_BUILD_OUTPUT_BASE_ABS_DIR!\*" "!K_PKG_BIN!" /S /I /Y
	
	REM - Copy in the libusb-win32 includes
	IF EXIST "!K_LIBUSB0_DEP_DIR!" CALL .\make_tasks\deploy_dep.cmd "!K_LIBUSB0_DEP_DIR!" "!K_PKG!" "!K_LIBUSB0_NAME!" ".\make_tasks\!K_LIBUSB0_NAME!.dep.lst"

	REM - Copy in the libusb-1.x includes
	IF EXIST "!K_LIBUSB10_DEP_DIR!" CALL .\make_tasks\deploy_dep.cmd "!K_LIBUSB10_DEP_DIR!" "!K_PKG!" "!K_LIBUSB10_NAME!" ".\make_tasks\!K_LIBUSB10_NAME!.dep.lst"
	
	REM - Copy in the libusbK includes
	CALL .\make_tasks\deploy_dep.cmd "!K_LIBUSBK_DEP_DIR!" "!K_PKG!" "!K_LIBUSBK_NAME!" ".\make_tasks\!K_LIBUSBK_NAME!.dep.lst"
	
	REM - generate def files to the gcc lib dir.
	REM CALL :MakeOutputDefFiles "\dll\x86\*.dll" "\lib\gcc"

	REM - generate gcc libs from .def files.
	REM CALL :MakeGccLibs "\lib\gcc\*.def" "\lib\gcc"
	
	REM - Move the customized libusbK inf-wizard files into the libwdi directory.
	CALL ".\src\libwdi\copy-to-libwdi.cmd" "!K_LIBWDI_DIR!"
	
	REM - Build libwdiK and inf-wizardK.
	IF EXIST "!K_LIBWDI_DIR!\libwdi\embedded.h" DEL /Q "!K_LIBWDI_DIR!\libwdi\embedded.h"
	IF EXIST "!K_LIBWDI_DIR!\Win32\Release\lib\libwdi.lib" DEL "!K_LIBWDI_DIR!\Win32\Release\lib\libwdi.lib"
	SET CL=/DLIBUSBK_DIR=\"!K_PKG_BIN:\=/!\" /DLIBUSB0_DIR=\"!K_LIBUSB0_DEP_DIR:\=/!\" /DDDK_DIR=\"!G_WDK_DIR:\=/!\"
	!G_DEVENV_EXE! "!K_LIBWDI_DIR!\libwdi_2008.sln" /build "Release|Win32" /project "libwdi (static)"
	IF "!ERRORLEVEL!" NEQ "0" (
		ECHO [BUILD ERROR] - libwdi_2008
		goto SetError
	)
	SET CL=
	COPY /Y "!K_LIBWDI_DIR!\Win32\Release\lib\libwdi.lib" ".\src\inf-wizard2\lib\libwdi.lib"
	!G_DEVENV_EXE! "!K_LIBWDI_DIR!\libwdi_2008.sln" /clean "Release|Win32" /project "libwdi (static)"

	COPY /Y "!K_LIBWDI_DIR!\libwdi\libwdi.h" ".\src\inf-wizard2\src\libwdi.h"
	!G_DEVENV_EXE! ".\src\inf-wizard2\InfWizard.sln" /build "Release|Win32" /project "InfWizard"
	IF "!ERRORLEVEL!" NEQ "0" (
		ECHO [BUILD ERROR] - InfWizard
		goto SetError
	)
	SET CL=
	COPY /Y ".\src\inf-wizard2\Release\InfWizard.exe" "!K_PKG!\libusbK-inf-wizard.exe"
	!G_DEVENV_EXE! ".\src\inf-wizard2\InfWizard.sln" /clean "Release|Win32" /project "InfWizard"

	REM PUSHD !CD!
	REM CD /D "!K_LIBWDI_DIR!"
	REM SET C_DEFINES=/DLIBUSBK_DIR=\"!K_PKG_BIN:\=/!\" /DLIBUSB0_DIR=\"!K_LIBUSB0_DEP_DIR:\=/!\" /DINFWIZARD_LIBUSBK=1
	REM SET RC_DEFINES=!C_DEFINES!
	REM CALL ddk_build inf_wizard_only
	REM SET C_DEFINES=
	REM SET RC_DEFINES=
	REM POPD
	REM COPY /Y "!K_LIBWDI_DIR!\examples\inf-wizard.exe" "!K_PKG!\libusbK-inf-wizard.exe"

	IF NOT EXIST "!K_PKG!\libusbK-inf-wizard.exe" (
		ECHO ERROR - "!K_PKG!\libusbK-inf-wizard.exe" not found.
		GOTO :EOF
	)
	
	PUSHD !CD!
	CD .\doc
	CALL make_dist_docs.cmd
	POPD
	
	SET TOKVAR_LTAG=@
	SET TOKVAR_RTAG=@
	IF EXIST "!G_PACKAGE_ABS_DIR!\!K_LIBUSBK_SETUP_NAME!.iss" DEL "!G_PACKAGE_ABS_DIR!\!K_LIBUSBK_SETUP_NAME!.iss" >NUL
	!DCMD! -et ".\src\setup.iss.in" "!G_PACKAGE_ABS_DIR!\!K_LIBUSBK_SETUP_NAME!.iss"
	SET TOKVAR_LTAG=
	SET TOKVAR_RTAG=
	COPY /Y .\make_tasks\libusbK.bmp "!G_PACKAGE_ABS_DIR!\"
	!K_ISCC_EXE! "!G_PACKAGE_ABS_DIR!\!K_LIBUSBK_SETUP_NAME!.iss"

GOTO :EOF

:RenameOutputSubDirs
	IF EXIST "!G_BUILD_OUTPUT_BASE_ABS_DIR!%~2" RMDIR /S /Q "!G_BUILD_OUTPUT_BASE_ABS_DIR!%~2"
	MOVE /Y "!G_BUILD_OUTPUT_BASE_ABS_DIR!%~1" "!G_BUILD_OUTPUT_BASE_ABS_DIR!%~2"
	SHIFT /1
	SHIFT /1
	IF "%~1" NEQ "" GOTO RenameOutputSubDirs
GOTO :EOF


:SwapFile
	IF NOT EXIST "%~1" IF EXIST "%~1.%~2" MOVE /Y "%~1.%~2" "%~1"
	MOVE /Y "%~1" "%~1.%~2"
	COPY /Y "%~3" "%~1"
GOTO :EOF

:RestoreFile
	MOVE /Y "%~1.%~2" "%~1"
GOTO :EOF

:MoveOutputFiles
	SET __MoveOutputFiles_SrcPattern=!G_BUILD_OUTPUT_BASE_ABS_DIR!%~1
	SET __MoveOutputFiles_Dst=!G_BUILD_OUTPUT_BASE_ABS_DIR!%~2
	FOR /F "usebackq eol=; tokens=* delims=" %%A IN (`DIR /A-D-H-R-S /S /B "!__MoveOutputFiles_SrcPattern!"`) DO (
		SET __SrcDir=%%~dpA
		SET __DstName=%%~nxA
		SET __DstFile=!__MoveOutputFiles_Dst!\!__DstName!
		IF NOT EXIST "!__MoveOutputFiles_Dst!" MKDIR "!__MoveOutputFiles_Dst!"
		ECHO Moving %%~A to !__DstFile!
		MOVE /Y "%%~A" "!__DstFile!" > NUL
	)
	SHIFT /1
	SHIFT /1
	IF "%~1" NEQ ""	GOTO MoveOutputFiles
GOTO :EOF

:MakeOutputDefFiles
	SET __MakeOutputDefFiles_SrcPattern=!G_BUILD_OUTPUT_BASE_ABS_DIR!%~1
	SET __MakeOutputDefFiles_Dst=!G_BUILD_OUTPUT_BASE_ABS_DIR!%~2
	FOR /F "usebackq eol=; tokens=* delims=" %%A IN (`DIR /A-D-H-R-S /S /B "!__MakeOutputDefFiles_SrcPattern!"`) DO (
		SET __SrcDir=%%~dpA
		SET __DstName=%%~nA.def
		SET __DstFile=!__MakeOutputDefFiles_Dst!\!__DstName!
		IF NOT EXIST "!__MakeOutputDefFiles_Dst!" MKDIR "!__MakeOutputDefFiles_Dst!"
		ECHO Generating .def file for %%~A..
		CALL pexports -o "%%~A">"!__DstFile!"
	)
	SHIFT /1
	SHIFT /1
	IF "%~1" NEQ ""	GOTO MakeOutputDefFiles
GOTO :EOF

:MakeGccLibs
	SET __MakeGccLibs_SrcPattern=!G_BUILD_OUTPUT_BASE_ABS_DIR!%~1
	SET __MakeGccLibs_Dst=!G_BUILD_OUTPUT_BASE_ABS_DIR!%~2
	FOR /F "usebackq eol=; tokens=* delims=" %%A IN (`DIR /A-D-H-R-S /S /B "!__MakeGccLibs_SrcPattern!"`) DO (
		SET __SrcDir=%%~dpA
		SET __DstName=%%~nA.a
		SET __DstFile=!__MakeGccLibs_Dst!\!__DstName!
		IF NOT EXIST "!__MakeGccLibs_Dst!" MKDIR "!__MakeGccLibs_Dst!"
		ECHO Generating gcc libs for %%~A..
		CALL "!G_MINGW_W32_DIR!\bin\dlltool.exe" --output-lib "!__DstFile!" --input-def "%%~A"
	)
	SHIFT /1
	SHIFT /1
	IF "%~1" NEQ ""	GOTO MakeGccLibs
GOTO :EOF

REM :: Called when a build error or other unrecoverable error occurs; notifies the parent make.cmd there was problems.
:SetError
	ENDLOCAL
	SET BUILD_ERRORLEVEL=1
	IF DEFINED E_SND_FILE (
		IF EXIST "!E_SND_FILE!" (
			ECHO 1 >!E_SND_FILE!
		)
	)
GOTO :EOF
