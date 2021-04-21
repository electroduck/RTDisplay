# RTDisplay
This Win32 application displays a greyscale image read in from a serial port, pipe, or other device. It provides a GUI for selecting the serial format (data bits, parity, baud rate, etc.) and continually displays new data from the device as it comes in.

The purpose of this program is to be able to see if the data stream from the device contains a valid image even before all the data has come in. Initially, the image is blank, and as data comes in, it is filled out.

In addition to the RTDisplay program, there is also a small program (one C file) that continuously sends image data to a pipe. This can be used to test the main program by asking it to read data from that pipe.

## Building
Use CMake to generate whatever build system you need (VS project, makefiles, etc.) and then build it normally.

Both RTDisplay and the test program can be compiled with no C standard library (they do not use any standard functions). If you want to do this, use whatever options are necessary for the build system you selected.

## License
Copyright (c) 2021 Electroduck (root@electroduck.com)

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.