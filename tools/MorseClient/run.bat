@echo off
title Morse Trainer Client
cd /d "%~dp0"
java -jar MorseClient.jar
if errorlevel 1 (
    echo.
    echo Java not found or wrong version.
    echo Install JDK 11 or later from https://adoptium.net
    echo Make sure to choose the x64 installer for Intel/AMD PCs.
    pause
)
