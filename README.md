
# Cassette Tape Emulator for Motorola MEK6802D5

A few years back, I picked up a Motorola D5 off of ebay.  I got it for nostalgic reasons.  You see, the first programming language that I was taught was Motorola 6800 assembly on a Motorola D2 and later learned microprocessor interfacing on a Motorola D5 (MEK6802D5).  I have fond memories of those days and it was the D2/D5 that gave me a love for firmware.  

![alt text](./images/mek6802d5.jpg?raw=true "MEK6802D5")

The D5 is very "Old School".  When I learned to program the 6800/6802, I had to write out my program long hand, convert the opcodes/operands in to machine code which also required calculating out any offsets for branches, and then enter my code one byte at a time.  This is very slow and tedious, but for one thing, once you've mastered the challenge, you truly do understand the workings of the microprocessor and its peripherals.  This is a skill that I've used through out my entire carrier as a hardware/software/firmware engineer and one skill that I'm grateful to have learned.  

The problem is that once you've entered your program, when you power down the D5, your program is lost and you have to re-enter your program one byte at a time all over again the next time you power up the D5.  To get around this, the engineers at Motorola included a boot-rom monitor that allows you to save your program to a cassette tape and then you can re-load your program from that tape at a later time.  But you still have to load the program the first time one byte at a time, which is a slow and error prone processes.  

Now for the reason for this repository.  To get around this issue of having to enter your program one byte at a time, I decided to develope a cassette tape emulator.  My approach is to first assemble the code using the Motorola 6800 assembler and the load the code through the boot-rom Load command as if it was loading the code from a cassette tape.  I first experimented with a couple of designs of my own with some limited success but then a came across the Bit Boffer!  The Bit Boffer is a design for a cassette tape interface for microprocessors that first appeared in the March 1979 issue of Byte Magazine and was written by Don Lancaster (of CMOS Cookbook fame).  This is an 
"Old School" design that uses discreet components interfaced to a UART.  GREAT!  This is perfect for what I need to interface to the D5.  I've included the full article in the ./doc directory if you care to read it.  The schematic for the Bit Boffer looks like this:

![alt text](./images/bit-boffer_schematic.jpg?raw=true "Bit Boffer Schematic")

Please be aware that there are a couple of errors in the schematic. The first is that the output of IC6 is pin 6.  The other error has to do with the wiring of IC4C and IC4D as noted below:

![alt text](./images/ic4-correction.jpg?raw=true "IC4 Correction")

In my version of the Bit Boffer, I used two Teensy 4.0 modules, one for the transmitter and one for the receiver.  I also needed to buffer the output of the transmitter through an CA3130 to be able to get the output signal at the correct amplitude and DC offset for the input of the D5.  Another slight design change is that I had to use transistors to level shift the signals from the Teensy's 3.3V output to the required 5V of the Bit Boffer (the minimum voltage for the CA3130 is 5V). 

My completed circuit for the Bit Boffer looks like this:

![alt text](./images/bit-boffer_circuit.jpg?raw=true "Bit Boffer Circuit")

There are a number of different parts to this repository that make up everything that you should need in order to build the Bit Boffer Tape Drive Emulator.

Here is a list of the different pieces of firmware and applications that make up this package:

| Directory | Description |
| ---| --- |
| transmitter       | The firmware for the Transmitter Teensy. |
| receiver          | The firmware for the Receiver Teensy. |
| test-bit-boffer   | Application programs for testing the Bit Boffer.  This requires the output of the transmitter to be connected directly to the input of the receiver. |
| bit-boffer-writer | Application program that's used to load a *.s19 file onto the D5.  The *.s19 file is the output from the Motorola 6800 Assembler that you can find in one of my other repositories. |
| doc | Documentation directory.  Currently has D5 manual and Bit Buffer article. |
