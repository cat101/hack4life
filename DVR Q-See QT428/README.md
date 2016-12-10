# Obtaining root on a Q-See-QT428-418-5 CCTV DVR
I have had this DVR for several years with no problems. My only disappointment was the lack of web APIs to integrate it into other home automation projects. I had read that older versions of the FW (I have 3.2.0) had a telnet deamon enabled with some default password. I have also read recently that many DVR have been exploited, but I did not want to go into a guessing game so I decided to do a hardware hack.

## DVR hardware overview
First let's look at some pics to identify the hardware

<img src="images/QT428%20Serial%20console%20hack%2000.jpg" width="30%"><img src="images/QT428%20Serial%20console%20hack%2001.jpg" width="30%"><img src="images/QT428%20Serial%20console%20hack%2002.jpg" width="30%">

Here is the inside. Based on the marking I figured out that this model has been sold under other brands like *BV security 2308SE-SL DVR* and *Cop USA DVR2308SE-SL*. In particular Q-See seems to be the low end line of DAHUA.

<img src="images/QT428%20Serial%20console%20hack%2003.jpg" width="50%"><img src="images/QT428%20Serial%20console%20hack%2004.jpg" width="50%">
<img src="images/QT428%20Serial%20console%20hack%2005.jpg" width="50%"><img src="images/QT428%20Serial%20console%20hack%2006.jpg" width="50%">
<img src="images/QT428%20Serial%20console%20hack%2007.jpg" width="50%"><img src="images/QT428%20Serial%20console%20hack%2008.jpg" width="50%">

And finally what I was looking for a UART port!!

<img src="images/QT428%20Serial%20console%20hack%2009.jpg" width="50%">

Using an oscilloscope I was able to determine the pin-out as follow (from left to right)

1. VCC 3.3v (square hole)
2. RX (3.3v levels)
3. TX (3.3v levels)
4. GND/Chassis

Then I hooked it up to a USB-TTL dongle and figured out that the communication speed was 115kbps. and more important we got a **root console!!!!**

<img src="images/QT428%20Serial%20console%20hack%2011.jpg" width="50%">

## Understanding the firmware
Using putty as a serial terminal I was able to tell that the firmware is based on hilinux and in particular that the hardware architecture is based on the hi3515 chip.

```bash
$ uname -a 

 Linux (none) 2.6.24-rt1-hi3515v100 #201010220920 Thu Mar 15 03:54:45 EST 2012 armv5tejl unknown
```

```bash
$ cat /proc/cpuinfo 

 Processor       : ARM926EJ-S rev 5 (v5l) 

 BogoMIPS        : 219.54 

 Features        : swp half thumb fastmult edsp java 

 CPU implementer : 0x41 

 CPU architecture: 5TEJ 

 CPU variant     : 0x0 

 CPU part        : 0x926 

 CPU revision    : 5 

 Cache type      : write-back 

 Cache clean     : cp15 c7 ops 

 Cache lockdown  : format C 

 Cache format    : Harvard 

 I size          : 16384 

 I assoc         : 4 

 I line length   : 32 

 I sets          : 128 

 D size          : 16384 

 D assoc         : 4 

 D line length   : 32 

 D sets          : 128 

 Hardware        : hi3515v100 

 Revision        : 0000 

 Serial          : 0000000000000000
```

The boot sequence is

1. /etc/init.d/rcS
1. /mnt/mtd/boot.sh
1. /mnt/mtd/dep2.sh
        
All the DVR software is under /mnt/mtd/ and some runtime parts get unpacked on boot to /nfsdir/

# Enabling a persistent root access
Since `telnetd` was already installed on the firmware image all I had to do was add `/usr/sbin/telnetd` to `/etc/init.d/rcS` and change the root password

# References
* [CCF-paper-Forensic-reliabilty-DVR](http://www.i-1.nl/blog/wp-content/uploads/CCF-paper-Forensic-reliabilty-DVR.pdf)
* [Full boot log (taken from the console)](bootlog.txt)
