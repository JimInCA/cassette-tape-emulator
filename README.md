
# Cassette Tape Emulator for Motorola MEK6802D5

A few years back, I picked up a Motorola D5 off of ebay.  I got it for nostalgic reasons.  You see, the first programming language that I was taught was Motorola 6800 assembly on a Motorola D2 (MEK6800D2) and later learned microprocessor interfacing on a Motorola D5 (MEK6802D5).  I have fond memories of those days and it was the D2/D5 that gave me a love for firmware.  

![alt text](./images/mek6802d5.jpg?raw=true "MEK6802D5")

The D5 is very "Old School".  When I learned to program the 6800/6802, I had to write out my program long hand, convert the opcodes/operands into machine code which also required calculating out any offsets for branches, and then enter my code one byte at a time through the D5's keypad.  This is very slow and tedious, but for one thing, once you've mastered the challenge, you truly do understand the workings of the microprocessor and its peripherals.  This is a skill that I've used through out my entire carrier as a hardware/software/firmware engineer and one skill that I'm grateful to have learned.  

The problem is that once you've entered your program, when you power down the D5, your program is lost and you have to re-enter your program one byte at a time all over again the next time you power up the D5.  To get around this, the engineers at Motorola included a boot-rom monitor that allows you to save your program to a cassette tape and then you can re-load your program from that tape at a later time.  But you still have to load the program the first time one byte at a time, which is a slow and error prone process.

Bit Boffer
---

Now for the reason for this repository.  To get around this issue of having to enter your program one byte at a time, I decided to develope a cassette tape emulator.  My approach is to first assemble the code using the Motorola 6800 assembler and then load the code through the boot-rom Load command as if it was loading the code from a cassette tape.  I first experimented with a couple of designs of my own with some limited success but then a came across the Bit Boffer!  The Bit Boffer is a design for a cassette tape interface for microprocessors that first appeared in the March 1976 issue of Byte Magazine and was written by Don Lancaster (of CMOS Cookbook fame).  This is an "Old School" design that uses discreet components interfaced to a UART.  GREAT!  This is perfect for what I need to interface to the D5.  I've included the full article in the ./doc directory if you care to read it.  The schematic for the Bit Boffer looks like this:

![alt text](./images/bit-boffer_schematic.jpg?raw=true "Bit Boffer Schematic")

Please be aware that there are a couple of errors in the schematic. The first is that the output of IC6 is pin 6.  The other error has to do with the wiring of IC4C and IC4D as noted below:

![alt text](./images/ic4-correction.jpg?raw=true "IC4 Correction")

In my version of the Bit Boffer, I used two Teensy 4.0 modules, one for the transmitter and one for the receiver.  I also needed to buffer the output of the transmitter through a CA3130 to be able to get the output signal at the correct amplitude and DC offset for the input of the D5.  Another slight design change is that I had to use transistors to level shift the signals from the Teensy's 3.3V output to the required 5V of the Bit Boffer (the minimum voltage for the CA3130 is 5V). 

My completed circuit for the Bit Boffer looks like this:

![alt text](./images/bit-boffer_circuit.jpg?raw=true "Bit Boffer Circuit")

There are a number of different parts to this repository that make up everything that you should need in order to build the Bit Boffer Tape Drive Emulator.

Here is a list of the different pieces of firmware and applications that make up this package:

| Directory | Description |
| ---| --- |
| transmitter       | The firmware for the Transmitter Teensy. |
| receiver          | The firmware for the Receiver Teensy. |
| test-bit-boffer   | Application programs for testing the Bit Boffer.  This requires the output of the transmitter to be connected directly to the input of the receiver. |
| bit-boffer-writer | Application program that's used to load a *.s19 file onto the D5.  The *.s19 file is the output from the Motorola 6800 Assembler that you find in this, one of my other repositories [Motorola 6800 Assembler](https://github.com/JimInCA/motorola-6800-assembler).|
| doc | Documentation directory.  Currently has the manual for the MEK6802D5 and Bit Buffer article. |

Transmitter
---
The transmitter firmware runs on a Teensy 4.0 which is Arduino compatible.  You'll need to visit the PJRC website and install the Teensyduino software add-on along with the Arduino IDE if you don't already have them installed.  The PJRC web site has all of the information that you should need on how to do this part of the process.

The transmitter's firmware is a modified version of the Teensy's USBtoSerial example program.  The transmitter requires an external 19.2KHz clock which is provided by the Teensy.  The 19.2KHz clock is generated through an ISR that uses one of the Teensy's timers to generate the interrupt for the ISR.  I also needed to modify the setup for the external UART to set it to a baud rate of 300bps, no parity, and two stop bits which makes it compatible with the Motorola D5's Load routine. 

Receiver
---
The receiver firmware also runs on a Teensy 4.0 with the same configuration as described for the transmitter.  The receiver firmware is also based on the Teensy's USBtoSerial example.  The first modification for the receiver's firmware was to set up the external UART for 300bps, no parity, and two stop bits to make it compatible with the Motorola D5's Punch routine.  The only other modification was to convert the hex data that's received on the UART RX pin to ascii and then send the ascii string to the USB to Serial port.  This allows you to monitor the data using a UART terminal window such as Tera Term.  

test-bit-boffer
---
The test-bit-boffer application program runs on a Widows host and can be built using gcc.  I also provided a solution file so that you can build the application under Visual Studio if you so desire.

test-bit-boffer's help menu lists the usage and arguments for the application as shown below.

```
$ ./bin/test-bit-boffer.exe -h
usage: test-bit-boffer [-h] -i COMPORT [-o COMPORT] [-b BAUDRATE] [-t TESTNUM] [-n LOOPNUM]

arguments:
  -h             Show this help message and exit.
  -i COMPORT     Transmitter COM Port.
  -o COMPORT     Receiver COM Port when TESTNUM > 0.
  -b BAUDRATE    Desired baudrate, default: 300.
  -t TESTNUM     Desired test to be executed.
                 0: Generate a count from 0x00 to 0xff and send to transmitter.
                    Connect a uart terminal window to receiver's com port.
                 1: Generate a count from 0x00 to 0xff, send to transmitter
                    and verify that the receiver received the correct data.
                 2: Send 'n' number of 0xff markers to transmitter and verify on receiver.
                 3: Send code for USED5 program to transmitter and verify on receiver.
                 4: Combine test 2 followed by test 3. This emulates a Load sequence.
                 5: Generate 'n' number of random bytes, send to transmitter and
                    verify on receiver.
  -n LOOPNUM     Number of test cycles in test loop.
```

All tests require that you connect the output of the transmitter directly to the input of the receiver.  This provides the ability for closed-loop testing.  

An example execution of test-bit-boffer follows:
```
$ ./bin/test-bit-boffer.exe -i COM22 -o COM9 -b 300 -t 5 -n 8 -v 1
Successfully connected to UART on port COM22 at baud rate 300.
Connected to transmitter port COM22 at baudrate 300
Successfully connected to UART on port COM9 at baud rate 300.
Connected to receiver port COM9 at baudrate 300
sent 0x43 -> received 0x43
sent 0xe3 -> received 0xe3
sent 0xdf -> received 0xdf
sent 0x86 -> received 0x86
sent 0x30 -> received 0x30
sent 0x06 -> received 0x06
sent 0xf8 -> received 0xf8
sent 0xf4 -> received 0xf4
Test 5 Passed!
```

bit-boffer-writer
---
bit-boffer-writter is the application that you'll need to run to load code into the Motorola D5's ram.  It too can be built with either gcc or Visual Studio.

bit-boffer-writter's help menu lists the usage and arguments for the application as shown below: 

```
$  ./bin/bit-boffer-writer.exe -h
usage: bit-boffer-writer [-h] -f FILE -p COMPORT [-b BAUDRATE] [-c NUMMARKS]

arguments:
  -h             Show this help message and exit.
  -f FILE        Filename of S-Record input file.
  -p COMPORT     COM Port to which the device is connected.
  -b BAUDRATE    Desired baudrate, default: 300.
  -c NUMMARKS    Number of marker cycles. default: 819.
```

The expected output from running bit-boffer-writer should look like the following example.

```
$ ./bin/bit-boffer-writer.exe -f ./test/used5.s19 -p COM22 -b 300 -c 1024
Successfully connected to UART on port COM22 at baud rate 300.
DCB is ready for use.
Sending file ./test/used5.s19 to port COM22 at baudrate 300
```

That's it for now and most of all, have fun with all of your projects.