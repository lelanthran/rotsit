@echo off

mkdir include
:start
if "%1"=="" goto end
echo Working with %1
for /F "tokens=1 delims=\" %%I in ("%1") do (
   mkdir include\%%I
   echo copy %1 include\%%I
   copy %1 include\%%I
)
shift
goto start

:end
