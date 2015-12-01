@ECHO OFF
SETLOCAL
SET EL=0

ECHO =========== %~f0 ===========

::----------- some debug information ----------------
IF DEFINED APPVEYOR (ECHO on AppVeyor) ELSE (ECHO NOT on AppVeyor)
ECHO APPVEYOR_REPO_COMMIT_MESSAGE^: %APPVEYOR_REPO_COMMIT_MESSAGE%
ECHO mapnik sdk:^ %MAPNIK_GIT%
ECHO CLIPPER_REVISION=7484da1
ECHO PROTOZERO_REVISION=v1.0.0
ECHO configuration^: %configuration%
ECHO platform^: %platform%
ECHO msvs_toolset^: %msvs_toolset%
SET BUILD_TYPE=%configuration%
SET BUILDPLATFORM=%platform%
SET TOOLS_VERSION=%msvs_toolset%.0
ECHO NUMBER_OF_PROCESSORS^: %NUMBER_OF_PROCESSORS%
ECHO RAM [MB]^:
powershell "get-ciminstance -class 'cim_physicalmemory' | %% { $_.Capacity/1024/1024}"
IF %ERRORLEVEL% NEQ 0 GOTO ERROR
::----------------------------------------


SET PATH=C:\Program Files\7-Zip;%PATH%


::--------------- download mapnik SDK -------------------
SET MAPNIK_SDK_URL=https://mapnik.s3.amazonaws.com/dist/dev/mapnik-win-sdk-%MAPNIK_GIT%-%platform%-%msvs_toolset%.0.7z
ECHO fetching mapnik sdk^: %MAPNIK_SDK_URL%
IF EXIST mapnik-sdk.7z (ECHO already downloaded) ELSE (powershell Invoke-WebRequest "${env:MAPNIK_SDK_URL}" -OutFile mapnik-sdk.7z)
IF %ERRORLEVEL% NEQ 0 GOTO ERROR
ECHO extracting mapnik sdk
IF EXIST mapnik-sdk (ECHO already extracted) ELSE (7z -y x mapnik-sdk.7z | %windir%\system32\FIND "ing archive")
IF %ERRORLEVEL% NEQ 0 GOTO ERROR
::------------------------------------------------------


:: ---------------- clone gyp ---------------
IF EXIST deps\gyp ECHO gyp already cloned && GOTO GYP_ALREADY_HERE

CALL git clone https://chromium.googlesource.com/external/gyp.git deps/gyp
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

:GYP_ALREADY_HERE
cd deps\gyp
git fetch
git pull
cd ..\..
::-------------------------------------

GOTO DONE

:ERROR
ECHO ~~~~~~~~~~~~ ERROR %~f0 ~~~~~~~~~~~~
ECHO ERRORLEVEL^: %ERRORLEVEL%
SET EL=%ERRORLEVEL%

:DONE
ECHO =========== DONE %~f0 ===========

EXIT /b %EL%
