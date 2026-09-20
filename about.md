# Timerres - WINDOWS ONLY

Why install an external program that has to run in the background when you can just put it in the game instead?

This is a Windows-only mod that lets you change the Windows timer resolution ingame

Apps like Discord and Chrome already change the Resolution to 1 ms but this mod uses the native Windows API `ntdll.dll`, which allows requesting a minimum of 0.1 ms. Most systems only support 0.5 ms as a minimum though.

## What even is the Timer Resolution you ask?

The Timer Resolution is responsible for I/O and mostly frame-pacing (via the Sleep function) on Windows. Reducing the value to it's lowest can \*potentially\* improve I/O (Keyboard \& Mouse) input latency, though the main impact is probably less stutters because of better frame-pacing.


## Notes

The actual timer resolution is determined by Windows and other programs running on the system. The requested value may not always be the value Windows uses.

Values below 0.5 ms are generally unsupported and may be ignored or changed by Windows.
