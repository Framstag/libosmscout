@echo off
protoc.exe --cpp_out=%1 --proto_path=%1 %1%2.proto
if exist %3%2.pb.h del /Q %3%2.pb.h
copy /Y %1%2.pb.h %3