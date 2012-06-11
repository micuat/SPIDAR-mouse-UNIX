SPIDAR-mouse-server for Linux, FreeBSD and Mac OS X
==============
Naoto Hieda <micuat@gmail.com>
June 11, 2012

About
------
Server program for SPIDAR-mouse ( spidar-string.com ).

After starting up, it will listen to localhost:8080.
It will accept "XForce,YForce,duration" format
where -1 <= XForce, YForce <= 1,
duration in milliseconds or duration < 0 for impulse.
Close connection to shut down.

Download
---------
It can be downloaded from github

`$ git clone git://github.com/micuat/SPIDAR-mouse-UNIX.git`

How to build
-------------------
Linux or FreeBSD:

    $ cd libusb
    $ make
    $ ./spidar-mouse-server

Mac:

    $ cd mac
    $ make
    $ ./spidar-mouse-server

Example code for Processing
--------
    // Includes example program by Tom Igoe
    // http://processing.org/reference/libraries/net/Client_write_.html
    import processing.net.*; 
    Client myClient; 
    int clicks;
    
    void setup() { 
      myClient = new Client(this, "127.0.0.1", 8080); 
    } 
    
    void mouseReleased() {
      clicks++;
      if( clicks > 5 ) {
        myClient.stop();
        exit();
      } else {
        myClient.write("0.5,0.5,"+clicks*10);
      }
    }
    
    void draw() { 
      // Change the background if the mouse is pressed
      if (mousePressed) {
        background(255);
      } else {
        background(0);
      }
    }

License
--------
See LICENSE.
