# Timerres

Why install an external program that has to run in the background when you can just put it in the game instead?

This is a Windows-only mod that lets you change the Windows timer resolution ingame

Apps like Discord and Chrome already change the Resolution to 1 ms but this mod uses the native Windows API `ntdll.dll`, which is capable of a minimum of 0.1 ms. Most systems only support 0.5 ms as a minimum though.

## Features

- Set the requested timer resolution from 0.1 ms to 15.625 ms.
- Built-in test to see the requested and current timer resolution.
- Tests `Sleep(1)` 100 times to show how much the resolution actually helps.
- Gives you the possibility of testing 0.1 ms on systems that may support them.

## Notes

The actual timer resolution is determined by Windows and other programs running on the system. The requested value may not always be the value Windows uses.

Values below 0.5 ms are generally unsupported and may be ignored or changed by Windows.
