#!/bin/sh
cd "$(dirname "$0")"
java --enable-native-access=ALL-UNNAMED -jar MorseClient.jar 2>/dev/null \
  || java -jar MorseClient.jar
