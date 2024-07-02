@echo off

call config.bat

set SCENE_NAME=bloom_test\bloom_test.hexray

start %REPOSITORY_EXECUTABLE_PATH% -scene "%REPOSITORY_SCENE_DIR%\%SCENE_NAME%"
