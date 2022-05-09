HP Printer Application
======================

![Version](https://img.shields.io/github/v/release/michaelrsweet/hp-printer-app?include_prereleases)
![Apache 2.0](https://img.shields.io/github/license/michaelrsweet/hp-printer-app)
[![LGTM Grade](https://img.shields.io/lgtm/grade/cpp/github/michaelrsweet/hp-printer-app)](https://lgtm.com/projects/g/michaelrsweet/hp-printer-app/context:cpp)
[![LGTM Alerts](https://img.shields.io/lgtm/alerts/github/michaelrsweet/hp-printer-app)](https://lgtm.com/projects/g/michaelrsweet/hp-printer-app/)


`hp-printer-app` implements printing for a variety of common PCL printers
connected via network or USB.  Features include:

- A single executable handles spooling, status, and server functionality.
- Multiple printer support.
- Each printer implements an IPP Everywhere™ print service and is compatible
  with the driverless printing support in Android™, Chrome OS™, iOS®, Linux®,
  macOS®, and Windows® 10/11 clients.
- Each printer can directly print "raw", Apple/PWG Raster, and/or PNG files.
- Each printer automatically recovers from out-of-media, power loss, and
  disconnected/bad cable issues.

> Note: Please use the Github issue tracker to report issues or request
> features/improvements in `hp-printer-app`:
>
> <https://github.com/michaelrsweet/hp-printer-app/issues>


Requirements
------------

`hp-printer-app` depends on:

- A POSIX-compliant "make" program (both GNU and BSD make are known to work),
- A C99 compiler (both Clang and GCC are known to work),
- [PAPPL](https://www.msweet.org/pappl) 1.1 or later.
- [CUPS](https://openprinting.github.io/cups) 2.2 or later (for libcups).


Installing
----------

`hp-printer-app` is published as a snap for Linux.  Run the following command
to install it:

    sudo snap install hp-printer-app

A package file is included with all source releases on Github for use on macOS
10.14 and higher for both Intel and Apple Silicon.

If you need to install `hp-printer-app` from source, you'll need a "make"
program, a C99 compiler (Clang and GCC work), the CUPS developer files, and the
PAPPL developer files.  Once the prerequisites are installed on your system,
use the following commands to install `hp-printer-app` to "/usr/local/bin":

    make
    sudo make install

You can change the destination directory by setting the `prefix` variable on
the second command, for example:

    sudo make prefix=/opt/hp-printer-app install


Basic Usage
-----------

`hp-printer-app` uses a single executable to perform all functions.  The normal
syntax is:

    hp-printer-app SUB-COMMAND [OPTIONS] [FILES]

where "SUB-COMMAND" is one of the following:

- "add": Add a printer
- "cancel": Cancel one or more jobs
- "default": Get or set the default printer
- "delete": Delete a printer
- "devices": List available printers
- "drivers": List available drivers
- "jobs": List queued jobs
- "modify": Modify a printer
- "options": Lists the supported options and values
- "printers": List added printer queues
- "server": Run in server mode
- "shutdown": Shutdown a running server
- "status": Show server or printer status
- "submit": Submit one or more files for printing

You can omit the sub-command if you just want to print something, for example:

    hp-printer-app somefile.pcl

The options vary based on the sub-command, but most commands support "-d" to
specify a printer and "-o" to specify a named option with a value, for example:

- `hp-printer-app -d myprinter somefile.pcl`: Print a file to the printer named
  "myprinter".
- `hp-printer-app -o media=na_letter_8.5x11in picture.jpg`: Print a picture to a US
  letter sheet.
- `hp-printer-app default -d myprinter`: Set "myprinter" as the default printer.

See the `hp-printer-app` man page for more examples.


Running the Server
------------------

Normally you'll run `hp-printer-app` in the background as a service for your
printer(s), either using the Snap package or the systemd service file:

    sudo systemctl enable hp-printer-app.service
    sudo systemctl start hp-printer-app.service

You can start it in the foreground with the following command:

    sudo hp-printer-app server -o log-file=- -o log-level=debug

Root access is needed on Linux when talking to USB printers, otherwise you can
run `hp-printer-app` without the "sudo" on the front.

On macOS you run the "HP Printer Application" program from the launcher, either
manually or as a login item.  An icon is shown in the menu bar that provides
access to the web interface and printers.


Supported Printers
------------------

The following printers are currently supported:

- HP LaserJet printers with PCL 5 language support
- Most HP DeskJet, OfficeJet, and Photosmart printers
- Laser printers with PCL 5 support from Canon, IBM, Lexmark, Kyocera, Ricoh,
  Xerox, etc.


Standards
---------

Through PAPPL, `hp-printer-app` implements PWG 5100.14-2013: IPP Everywhere™
for each printer, and has a partial implementation of PWG 5100.22-2019: IPP
System Service v1.0 for managing the print queues and default printer.


Legal Stuff
-----------

The HP Printer Application is Copyright © 2019-2022 by Michael R Sweet.

This software is licensed under the Apache License Version 2.0.  See the files
"LICENSE" and "NOTICE" for more information.
