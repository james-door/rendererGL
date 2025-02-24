@echo off

set filepath=%1
set linenumber=%2
set executable=%3
taskkill /IM remedybg.exe /F
start "" remedybg.exe -q %executable%
ping 127.0.0.1 -n 1 -w 200 >nul
remedybg.exe add-breakpoint-at-file %filepath% %linenumber%
remedybg.exe start-debugging

