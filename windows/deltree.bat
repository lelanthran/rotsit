@echo off
rd /s /q  %* 2> nul
del /s /q %* 2> nul
exit /b 0
