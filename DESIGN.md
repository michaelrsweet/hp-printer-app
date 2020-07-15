HP Printer Application
======================

**Way to write the file hp-printer-app.c(can be used as a recipe to write
any printer application).**

## pcl_t

*Per-job driver data*

PCL job data structure. It contains information like compression buffers,
output buffers, line skip counts.

## main()

*Main entry for the application*

Runs a standard main loop with a [system callback](#system_cb()). Also allows
the printer application to define its own usage callback and have an application
specific subcommand.

## pcl_callback()

*Driver callback function*

This function tells the printer application what functions to call when performing
job functions like starting a job, starting a page, writing a line, ending a page,
ending a job, printing a raw job. Driver capability information and defaults(such
as resolution, color etc.) are also provided here.

All the required information is stored in the `pappl_pdriver_data_t` structure. For
the entire list of attributes that can be provided, please look at the
`pappl_pdriver_data_t` structure.

## pcl_compress_data()

*Compress a line of graphics*

Compresses a line of graphics and writes it to the printer. Note that this is not a
mandatory function.

## pcl_identify()

*Identify printer function*

Identify a printer using display, flash, sound or speech.

## pcl_print()

*Print(file) function*

Used to print a raw job - called if the job format is the same as the format
specified by the [driver callback](#pcl_callback()).

## pcl_rendjob()

*End raster job function*

Called to end a raster job. The [job data](#pcl_t) set by
[pcl_rstartjob()](#pcl_rstartjob()) must be freed.

## pcl_rendpage()

*End raster page function*

Called each time a raster page is completed. Resets the buffers used by the driver. 

## pcl_rstartjob()

*Start raster job function*

Called when starting a job. The [job data](#pcl_t) is stored with the job.

## pcl_rstartpage()

*Start raster page function*

Called when starting a raster page. Information regarding the page is obtained
from the page header and attributes like resolution, margins, page size, orientation
and graphics are set appropriately.

## pcl_rwrite()

*Write a raster line function*

Writes a line of graphics. It also dithers the incoming line to the supported
color spaces(`CMYK` for color graphics and `K` for black and white graphics).

## pcl_setup()

*Setup drivers function*

Defines the list of print drivers and [driver callback](#pcl_callback()).

## pcl_status()

*Update printer status*

Updates the printer supply data.

## system_cb()

*System callback*

Creates a system object and allows restoring the previous system configuration(if any).