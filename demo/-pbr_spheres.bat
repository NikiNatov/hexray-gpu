@echo off

call config.bat

set SCENE_NAME=pbr_spheres\pbr_spheres.hexray

start %REPOSITORY_EXECUTABLE_PATH% -scene "%REPOSITORY_SCENE_DIR%\%SCENE_NAME%"
