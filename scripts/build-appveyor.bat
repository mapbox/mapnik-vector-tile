@ECHO OFF
SETLOCAL
SET EL=0

ECHO =========== %~f0 ===========


GOTO DONE

:ERROR
ECHO ~~~~~~~~~~~~ ERROR %~f0 ~~~~~~~~~~~~
ECHO ERRORLEVEL^: %ERRORLEVEL%
SET EL=%ERRORLEVEL%

:DONE
ECHO =========== DONE %~f0 ===========

EXIT /b %EL%
