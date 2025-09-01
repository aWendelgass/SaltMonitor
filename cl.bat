copy /Y "build\esp32.esp32.esp32\*.ino.merged.bin" ".\"
copy /Y "build\esp32.esp32.esp32\*.ino.bin" ".\"
del  /Q "build\esp32.esp32.esp32\*.*"

rmdir /S /Q build
