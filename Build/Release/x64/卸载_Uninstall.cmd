@ECHO OFF

%1 mshta vbscript:CreateObject("Shell.Application").ShellExecute("cmd.exe","/c %~s0 ::","","runas",1)(window.close)&&exit
cd /d "%~dp0"

regsvr32 /u "%~dp0ExplorerBgTool.dll"
taskkill /f /im explorer.exe & start explorer.exe
pause