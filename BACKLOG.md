# Backlog

## Current Milestone
Establish testing infrastructure and CI/CD.

## Priority: CRITICAL

### Testing Infrastructure
- [ ] Add native unit tests for Trainer logic (probability adjustment, speed analysis, queue management) — PlatformIO native test runner or desktop build with mock Arduino.h
- [ ] Add unit tests for MorseEngine (pattern lookup, timing calculations)
- [ ] Add unit tests for Storage (config serialization/deserialization)
- [ ] Determine test strategy — PlatformIO `test/` with `native` env, or extract pure logic into testable modules

### CI/CD
- [ ] Add GitHub Actions workflow (PlatformIO build on push/PR — both envs: nodemcuv2 + esp8266_oled)

## Priority: MEDIUM

### Code Quality
- [ ] Add PlatformIO linting (cppcheck or clang-tidy)
- [ ] Review sync_arduino.py — ensure it handles all edge cases for Arduino IDE compatibility

## Done
- [x] Bootstrap iterative session methodology
