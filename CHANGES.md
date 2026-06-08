Changes in HP Printer Application
=================================

v1.3.1 - YYYY-MM-DD
-------------------

- Improved PCL compression code.
- Fixed CMYK output.
- Fixed a few PCL 6 output bugs.
- Fixed the `--enable-experimental` configure option to enable the PCL 6 driver.


v1.3.0 - 2024-02-09
-------------------

- Switched to using a configure script.
- Updated documentation for current PAPPL.
- Added support for snap server configuration settings (Issue #21)
- Updated macOS and Linux binaries to use PAPPL 1.4.x.


v1.2.0 - 2022-05-15
-------------------

- Fixed default listen address when running as a server (Issue #7)
- Fixed the media size list which prevented A4 from showing up (Issue #8)
- Fixed dropped connections in the web browser when switching to HTTPS
  (Issue #9)
- Now offer more appropriate printer icons (Issue #10)
- Fixed a bug in the HP DeskJet driver (Issue #11)
- Fixed draft output (Issue #12)
- Fixed a crash bug when there was no default printer (Issue #14)
