#!/bin/bash
find source -type f \( -name "*.cxx" -or -name "*.hxx" -or -name "*.c" -or -name "*.h" -or -name "*.hxx.in" \) -and ! -name "Gorilla.*" > fileList.txt
uncrustify -c uncrustify.config -F fileList.txt --replace
find source -name "*.unc-backup~" -o -name "*.unc-backup.md5~" > fileList.txt
xargs -I{} sh -c 'rm {}' < fileList.txt
rm fileList.txt
