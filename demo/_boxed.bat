@echo off

call config.bat

set SCENE_NAME=boxed

start %ORIGINAL_HEXRAY_EXECUTABLE_PATH% %ORIGINAL_HEXRAY_SCENE_DIR%\%SCENE_NAME%.hexray
start %REPOSITORY_EXECUTABLE_PATH% -hexray-scene "%REPOSITORY_ORIGINAL_SCENE_DIR%\%SCENE_NAME%\%SCENE_NAME%.hexray"
