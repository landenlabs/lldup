@echo off

@echo Copy Release to d:\opt\bin
copy lldup-ms\x64\Release\lldup.exe d:\opt\bin\lldup.exe


@echo
@echo Compare md5 hash
cmp -h lldup-ms\x64\Release\lldup.exe d:\opt\bin\lldup.exe
ld -a d:\opt\bin\lldup.exe

@echo
@echo List all lldup.exe
ld -r -F=lldup.exe bin d:\opt\bin
