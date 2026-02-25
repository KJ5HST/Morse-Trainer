@echo off
title Morse Trainer Client
java -jar MorseClient.jar
if errorlevel 1 (
    echo.
    echo Java not found. Install JDK 11 or later from https://adoptium.net
    pause
)
