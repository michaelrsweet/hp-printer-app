HP Printer App Documentation
============================

HP Printer App v1.2.0
Copyright 2019-2022 by Michael R Sweet

`hp-printer-app` is licensed under the Apache License Version 2.0.  See the
files "LICENSE" and "NOTICE" for more information.


Table of Contents
-----------------

- [Overview](#overview)
- [Installation](#installation)
- [Basic Usage](#basic-usage)
- [Adding Printers](#adding-printers)
- [Printing Options](#printing-options)
- [Setting Default Options](#setting-default-options)
- [Running a Server](#running-a-server)
- [Server Web Interface](#server-web-interface)
- [Resources](#resources)


Overview
--------

`hp-printer-app` is an HP PCL printer application for macOS® and Linux®.  I
wrote it originally as sample code for [PAPPL](https://www.msweet.org/pappl)
and now maintain it as a standalone application.

Basically, `hp-printer-app` is a print spooler optimized for PCL printing.
It directly accepts "raw" print data as well as JPEG and PNG images and has
built-in "drivers" to send the print data to USB and network-connected PCL
printers.  The spooler also tries to keep the paper moving by merging jobs over
a single connection to the printer rather than starting and stopping like CUPS
does to support a wider variety of printers.

`hp-printer-app` supports all standard PCL paper sizes, paper trays, and 2-sided
printing modes.  Whenever possible, `hp-printer-app` will auto-detect the make
and model of your printer and its installed capabilities.  And you can configure
the default values of all options as well as manually configure the media that
is loaded in each printer.

`hp-printer-app` also offers a simple network server mode that makes any PCL
printers appear as IPP Everywhere™/AirPrint™/Mopria™ printers on your network.
Thus, any Android™, Chrome OS™, iOS®, Linux, macOS, or Windows 10/11 client can
use any PCL printer supported by `hp-printer-app`.  And you can, of course, send
jobs from `hp-printer-app` to an `hp-printer-app` server on the network.

Finally, `hp-printer-app` offers command-line and web-based monitoring of
printer and job status.


Installation
------------

`hp-printer-app` is published as a snap for Linux.  Run the following command to
install it:

    sudo snap install hp-printer-app

A package file is included with all source releases on Github for use on macOS
10.14 and higher for both Intel and Apple Silicon.

If you need to install `hp-printer-app` from source, you'll need a "make"
program, a C99 compiler (Clang and GCC work), the CUPS developer files, and the
PAPPL developer files.  Once the prerequisites are installed on your system, use
the following commands to install `hp-printer-app` to "/usr/local/bin":

    ./configure
    make
    sudo make install

The "configure" script supports the usual autoconf options - run:

    ./configure --help

to get a list of configuration options.


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

    hp-printer-app report.pcl

The options vary based on the sub-command, but most commands support "-d" to
specify a printer and "-o" to specify a named option with a value, for example:

- `hp-printer-app -d myprinter report.pcl`: Print a file to the printer named
  "myprinter".
- `hp-printer-app -o media=na_index-4x6_4x6in photo.jpg`: Print a photo to a
  4x6 page.
- `hp-printer-app default -d myprinter`: Set "myprinter" as the default printer.

You can find our more about each sub-command by reading its included man page,
for example the following command will explain all of the supported options for
the "submit" sub-command:

    man hp-printer-app-submit


Adding Printers
---------------

The "add" sub-command adds a new printer:

    hp-printer-app add -d PRINTER -v DEVICE-URI -m DRIVER-NAME

"PRINTER" is the name you want to give the print queue.  Printer names can
contain spaces and special characters, but if you do any printing from scripts
you probably want to limit yourself to ASCII letters, numbers, and punctuation.

"DEVICE-URI" is a "usb:", "snmp:", or "socket:" URI for the printer.  For
USB-connected label printers, use the "devices" sub-command to discover the URI
to use:

    hp-printer-app devices

For network-connected printers, print the configuration summary on your printer
to discover its IP address.  The device URI will then be "socket://" followed by
the IP address.  For example, a printer at address 192.168.0.42 will use the
device URI "socket://192.168.0.42".

Many network-connected printers also support discovery via DNS-SD and SNMP - use
the "devices" sub-command to discover these printers' device URIs.

Finally, the "DRIVER-NAME" is the name of the internal `hp-printer-app` driver
for the printer.  Use the "drivers" sub-command to list the available drivers:

    hp-printer-app drivers

For example, a common PCL laser printer uses the "hp_generic" driver:

    hp-printer-app add -d myprinter -v socket://192.168.0.42 -m hp_generic


Printing Options
----------------

The following options are supported by the "submit" sub-command:

- "-n NNN": Specifies the number of copies to produce.
- "-o media=SIZE-NAME": Specifies the media size name using the PWG media size
  self-describing name (see below).
- "-o media-source=TRAY-NAME": Specifies the tray to use such as 'tray-1' or
  'manual'.
- "-o media-type=TYPE-NAME": Specifies a media type name such as 'stationery',
  'photographic', or 'transparency'.
- "-o orientation-requested=none": Prints in portrait or landscape orientation
  automatically.
- "-o orientation-requested=portrait": Prints in portrait orientation.
- "-o orientation-requested=landscape": Prints in landscape (90 degrees counter-
  clockwise) orientation.
- "-o orientation-requested=reverse-portrait": Prints in reverse portrait
  (upside down) orientation.
- "-o orientation-requested=reverse-landscape": Prints in reverse landscape
  (90 degrees clockwise) orientation.
- "-o print-color-mode=bi-level": Prints black-and-white output with no shading.
- "-o print-color-mode=monochrome": Prints grayscale output with shading as
  needed.
- "-o print-content-optimize=auto": Automatically optimize printing based on
  content.
- "-o print-content-optimize=graphic": Automatically optimize printing for
  graphics like lines and barcodes.
- "-o print-content-optimize=photo": Automatically optimize printing for
  photos or other shaded images.
- "-o print-content-optimize=text": Automatically optimize printing for text.
- "-o print-content-optimize=text-and-graphic": Automatically optimize printing
  for text and graphics.
- "-o print-quality=draft": Print using the lowest quality and fastest speed.
- "-o print-quality=normal": Print using good quality and speed.
- "-o print-quality=high": Print using the best quality and slowest speed.
- "-o printer-resolution=NNNdpi": Specifies the print resolution in dots per
  inch.
- "-t TITLE": Specifies the title of the job that appears in the list produced
  by the "jobs" sub-command.

Media sizes use the PWG self-describing name format which looks like this:

    CLASS_NAME_WIDTHxLENGTHin
    CLASS_NAME_WIDTHxLENGTHmm

"CLASS" is "na" for North American media sizes, "iso" for international media
sizes, and "jis" for Japanese media sizes.  The standard sizes are:

- `iso_a3_297x420mm`: ISO A3
- `iso_a4_210x297mm`: ISO A4
- `iso_a5_148x210mm`: ISO A5
- `iso_b5_176x250mm`: ISO B5
- `iso_c5_162x229mm`: ISO C5 Envelope
- `iso_dl_110x220mm`: ISO DL Envelope
- `jis_b5_182x257mm`: JIS B5
- `na_ledger_11x17in`: US Tabloid
- `na_legal_8.5x14in`: US Legal
- `na_letter_8.5x11in`: US Letter
- `na_executive_7x10in`: US Executive
- `na_number-10_4.125x9.5in`: US #10 Envelope
- `na_monarch_3.875x7.5in`: US Monarch Envelope

You can get a list of supported values for these options using the "options"
sub-command:

    hp-printer-app options
    hp-printer-app options -d PRINTER


Setting Default Options
-----------------------

You can set the default values for each option with the "add" or "modify"
sub-commands:

    hp-printer-app add -d PRINTER -v DEVICE-URI -m DRIVER-NAME -o OPTION=VALUE ...
    hp-printer-app modify -D PRINTER -o OPTION=VALUE ...

In addition, you can configure the installed media and other printer settings
using other "-o" options.  For example, the following command configures the
paper that is installed in a HP LaserJet printer:

    hp-printer-app modify -d LaserJet \
      -o media-ready=na_letter_8.5x11in,iso_a4_210x297mm

Use the "options" sub-command to see which settings are supported for a
particular printer.


Running a Server
----------------

The "server" sub-command runs a standalone spooler.  The following options
control the server operation:

- "-o listen-hostname=HOSTNAME": Sets the network hostname to resolve for listen
  addresses - use "*" for the wildcard addresses.
- "-o server-hostname=HOSTNAME": Sets the network hostname to advertise.
- "-o server-name=DNS-SD-NAME": Sets the DNS-SD name to advertise.
- "-o server-port=NNN": Sets the network port number; the default is randomly
  assigned.
- "-o auth-service=SERVICE": Specifies a PAM service for remote authentication.
- "-o admin-group=GROUP": Specifies a group to use for remote authentication.
- "-o spool-directory=DIRECTORY": Specifies the directory to store print files.
- "-o log-file=FILENAME": Specifies a log file.
- "-o log-file=-": Specifies that log entries are written to the standard error.
- "-o log-file=syslog": Specifies that log entries are sent to the system log.
- "-o log-level=LEVEL": Specifies the log level - "debug", "info", "warn",
  "error", or "fatal".

> *Note:* When you install the `hp-printer-app` snap on Linux, the server is
> automatically run as root.  On macOS, running the "HP Printer App" application
> starts the server.  When you install from source, a `systemd` service file is
> installed but not activated - it can be used to automatically start
> `hp-printer-app` when the system boots.


Server Web Interface
--------------------

When you run a standalone spooler on a network hostname, a web interface can be
enabled that allows you to add, modify, and delete printers, as well as setting
the default printer.  To require authentication for the various web interface
operations, you set the PAM authentication service with the
`-o auth-service=SERVICE` option.  For example, to use the "cups" PAM service
with `hp-printer-app`, run:

    hp-printer-app -o server-name=HOSTNAME -o server-port=NNN -o auth-service=cups

By default, any user can authenticate web interface operations.  To restrict
access to a particular UNIX group, use the `-o admin-group=GROUP` option as
well.


Resources
---------

The following additional resources are available:

- `hp-printer-app` home page: <https://www.msweet.org/hp-printer-app>
- `hp-printer-app` Github project: <https://github.com/michaelrsweet/hp-printer-app>
