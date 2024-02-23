
# Enhanced SIDDump V0.1
by beachviking

Based on the original works of the authors cited below.
Adds various output options by introducing an 'm' option:

0. output to screen, with note information
1. output to screen, sid registers only
2. output to binary file, all sid registers per frame
3. output to c friendly include file, all sid registers per frame
4. output to binary file, all sid registers + timing HI/LO bytes per frame
5. output to screen, only output changed sid registers inc. timing HI/LO bytes
6. output to binary file, only output changed sid registers inc. timing HI/LO bytes

A simplistic strategy pattern has been used to make it easy to add new output
formats without changing the main code.

The -m6 output file option can be used by the example ESP32 register based SID player available here:
https://github.com/beachviking/arduino-sid-tools

This output format does a diff of all the SID registers(0-24) and outputs the following per frame(in binary):

ABC...

A = #number of registers changed for this frame
B = [reg nr, if any]
C = [reg value, if any]
....

'A' will always be written per frame.

For the first frame, all registers will be reported to give these their initial value.

Two "special" register numbers have been introduced with the LOW and HIGH numbers of the frame period(25 and 26), which together forms a 16 bit number which indicates the duration of the current frame, in uS. Many SID tunes use the default 50/60Hz timing, but others use custom CIA based timing. This can be used along with the desired sampling frequency to calculate the number of samples desired per frame.

With the introduction of this functionality, CIA timing based tunes can be reproduced properly.  Given that only changed registers are reported per frame, a smaller file size is also obtained.

_________________________________________________________
## SIDDump V1.08
by Lasse Oorni (loorni@gmail.com) and Stein Pedersen

Version history:

V1.0    - Original
V1.01   - Fixed BIT instruction
V1.02   - Added incomplete illegal opcode support, enough for John Player
V1.03   - Some CPU bugs fixed
V1.04   - Improved command line handling, added illegal NOP instructions, fixed
          illegal LAX to work again
V1.05   - Partial support for multispeed tunes
V1.06   - Added CPU cycle profiling functionality by Stein Pedersen
V1.07   - Support rudimentary line counting for SID detection routines
V1.08   - CPU bugfixes

Copyright (C) 2005-2020 by the authors. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.


