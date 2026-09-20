# Changelog

## v2.1.0

- Read through the docs more thoroughly and improved accordingly
- Cut source code in half by not using Handles for `ntdll`
- Now uses `ntdll` as a library, which makes the import of the function that changes the TimerResolution easier

## v2.0.0

- Rewrote the entire mod from scratch
- Removed the timer test

## v1.0.0

- Added configurable Windows timer resolution.
- Added support for changing the resolution while Geometry Dash is running.
- Added a timer test using 100 `Sleep(1)` calls.
- Added requested and actual timer resolution information to the test.
- Added support for timer resolution values from 0.1 ms to 15.625 ms.