@ECHO OFF
SETLOCAL
SET EL=0

ECHO =========== %~f0 ===========

SET msvs_toolset=14
SET platform=x64
SET configuration=Release


CALL scripts\build-appveyor.bat
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

GOTO DONE

:ERROR
ECHO ~~~~~~~~~~~~ ERROR %~f0 ~~~~~~~~~~~~
ECHO ERRORLEVEL^: %ERRORLEVEL%
SET EL=%ERRORLEVEL%

:DONE
ECHO =========== DONE %~f0 ===========

EXIT /b %EL%
