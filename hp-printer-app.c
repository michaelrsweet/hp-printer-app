//
// HP-Printer app for the Printer Application Framework
//
// Copyright © 2020-2022 by Michael R Sweet.
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

//
// Include necessary headers...
//

# include <pappl/pappl.h>
# include "icons.h"
# include <math.h>


//
// Types...
//

typedef struct pcl_s			// Job data
{
  size_t	linesize;		// Size of output line
  unsigned	width,			// Width
		height,			// Height
		xstart,			// First column on page/line
		xend,			// Last column on page/line
		ystart,			// First line on page
		yend;			// Last line on page
  unsigned char	*planes[4],		// Output buffers
		*comp_buffer;		// Compression buffer
  unsigned	num_planes,		// Number of color planes
		feed;			// Number of lines to skip
  int		compression;		// Compression mode
  bool		is_deskjet;		// DeskJet mode?
} pcl_t;


//
// Local globals...
//

static pappl_pr_driver_t pcl_drivers[] =   // Driver information
{   /* name */          /* description */	/* device ID */	/* extension */
  { "hp_deskjet",	"HP Deskjet",		NULL,		NULL },
  { "hp_generic",	"Generic PCL",		"CMD:PCL;",	NULL },
  { "hp_laserjet",	"HP LaserJet",		NULL,		NULL }
};

static const char * const pcl_hp_deskjet_media[] =
{       // Supported media sizes for HP Deskjet printers
  "na_legal_8.5x14in",
  "na_letter_8.5x11in",
  "na_executive_7x10in",
  "iso_a4_210x297mm",
  "iso_a5_148x210mm",
  "jis_b5_182x257mm",
  "iso_b5_176x250mm",
  "na_number-10_4.125x9.5in",
  "iso_c5_162x229mm",
  "iso_dl_110x220mm",
  "na_monarch_3.875x7.5in"
};

static const char * const pcl_generic_pcl_media[] =
{       // Supported media sizes for Generic PCL printers
  "na_ledger_11x17in",
  "na_legal_8.5x14in",
  "na_letter_8.5x11in",
  "na_executive_7x10in",
  "iso_a3_297x420mm"
  "iso_a4_210x297mm",
  "iso_a5_148x210mm",
  "jis_b5_182x257mm",
  "iso_b5_176x250mm",
  "na_number-10_4.125x9.5in",
  "iso_c5_162x229mm",
  "iso_dl_110x220mm",
  "na_monarch_3.875x7.5in"
};

static const char * const pcl_hp_laserjet_media[] =
{       // Supported media sizes for HP Laserjet printers
  "na_ledger_11x17in",
  "na_legal_8.5x14in",
  "na_letter_8.5x11in",
  "na_executive_7x10in",
  "iso_a3_297x420mm"
  "iso_a4_210x297mm",
  "iso_a5_148x210mm",
  "jis_b5_182x257mm",
  "iso_b5_176x250mm",
  "na_number-10_4.125x9.5in",
  "iso_c5_162x229mm",
  "iso_dl_110x220mm",
  "na_monarch_3.875x7.5in"
};


//
// Local functions...
//

static const char *pcl_autoadd(const char *device_info, const char *device_uri, const char *device_id, void *data);
static bool   pcl_callback(pappl_system_t *system, const char *driver_name, const char *device_uri, const char *device_id, pappl_pr_driver_data_t *driver_data, ipp_t **driver_attrs, void *data);
static void   pcl_compress_data(pappl_job_t *job, pappl_device_t *device, unsigned char *line, unsigned length, unsigned plane, unsigned type);
static bool   pcl_print(pappl_job_t *job, pappl_pr_options_t *options, pappl_device_t *device);
static bool   pcl_rendjob(pappl_job_t *job, pappl_pr_options_t *options, pappl_device_t *device);
static bool   pcl_rendpage(pappl_job_t *job, pappl_pr_options_t *options, pappl_device_t *device, unsigned page);
static bool   pcl_rstartjob(pappl_job_t *job, pappl_pr_options_t *options, pappl_device_t *device);
static bool   pcl_rstartpage(pappl_job_t *job, pappl_pr_options_t *options, pappl_device_t *device, unsigned page);
static bool   pcl_rwriteline(pappl_job_t *job, pappl_pr_options_t *options, pappl_device_t *device, unsigned y, const unsigned char *pixels);
static void   pcl_setup(pappl_system_t *system);
static bool   pcl_status(pappl_printer_t *printer);


//
// 'main()' - Main entry for the hp-printer-app.
//

int
main(int  argc,				// I - Number of command-line arguments
     char *argv[])			// I - Command-line arguments
{
  return (papplMainloop(argc, argv,
                        VERSION,
                        "Copyright &copy; 2020-2022 by Michael R Sweet. Provided under the terms of the <a href=\"https://www.apache.org/licenses/LICENSE-2.0\">Apache License 2.0</a>.",
                        (int)(sizeof(pcl_drivers) / sizeof(pcl_drivers[0])),
                        pcl_drivers, pcl_autoadd, pcl_callback,
                        /*subcmd_name*/NULL, /*subcmd_cb*/NULL,
                        /*system_cb*/NULL,
                        /*usage_cb*/NULL,
                        /*data*/NULL));
}


//
// 'pcl_autoadd()' - Auto-add PCL printers.
//

static const char *			// O - Driver name or `NULL` for none
pcl_autoadd(const char *device_info,	// I - Device name
            const char *device_uri,	// I - Device URI
            const char *device_id,	// I - IEEE-1284 device ID
            void       *data)		// I - Callback data (not used)
{
  const char	*ret = NULL;		// Return value
  int		num_did;		// Number of device ID key/value pairs
  cups_option_t	*did;			// Device ID key/value pairs
  const char	*cmd,			// Command set value
		*pcl;			// PCL command set pointer


  // Parse the IEEE-1284 device ID to see if this is a printer we support...
  num_did = papplDeviceParseID(device_id, &did);

  // Look at the COMMAND SET (CMD) key for the list of printer languages,,,
  if ((cmd = cupsGetOption("COMMAND SET", num_did, did)) == NULL)
    cmd = cupsGetOption("CMD", num_did, did);

  if (cmd && (pcl = strstr(cmd, "PCL")) != NULL && (pcl[3] == ',' || !pcl[3]))
  {
    // Printer supports HP PCL, now look at the MODEL (MDL) string to see if
    // it is one of the HP models or a generic PCL printer...
    const char *mdl;			// Model name string

    if ((mdl = cupsGetOption("MODEL", num_did, did)) == NULL)
      mdl = cupsGetOption("MDL", num_did, did);

    if (mdl && (strstr(mdl, "DeskJet") || strstr(mdl, "Photosmart")))
      ret = "hp_deskjet";		// HP DeskJet/Photosmart printer
    else if (mdl && strstr(mdl, "LaserJet"))
      ret = "hp_laserjet";		// HP LaserJet printer
    else
      ret = "hp_generic";		// Some other PCL laser printer
  }

  cupsFreeOptions(num_did, did);

  return (ret);
}


//
// 'pcl_callback()' - PCL callback.
//

static bool				// O - `true` on success, `false` on failure
pcl_callback(
    pappl_system_t         *system,	// I - System
    const char             *driver_name,// I - Driver name
    const char             *device_uri,	// I - Device URI (not used)
    const char             *device_id,	// I - IEEE-1284 device ID (not used)
    pappl_pr_driver_data_t *driver_data,// O - Driver data
    ipp_t                  **driver_attrs,
					// O - Driver attributes (not used)
    void                   *data)	// I - Callback data (not used)
{
  int   i, j;				// Looping variables
  static pappl_dither_t	dither =	// Blue-noise dither array
  {
    { 111,  49, 142, 162, 113, 195,  71, 177, 201,  50, 151,  94,  66,  37,  85, 252 },
    {  25,  99, 239, 222,  32, 250, 148,  19,  38, 106, 220, 170, 194, 138,  13, 167 },
    { 125, 178,  79,  15,  65, 173, 123,  87, 213, 131, 247,  23, 116,  54, 229, 212 },
    {  41, 202, 152, 132, 189, 104,  53, 236, 161,  62,   1, 181,  77, 241, 147,  68 },
    {   2, 244,  56,  91, 230,   5, 204,  28, 187, 101, 144, 206,  33,  92, 190, 107 },
    { 223, 164, 114,  36, 214, 156, 139,  70, 245,  84, 226,  48, 126, 158,  17, 135 },
    {  83, 196,  21, 254,  76,  45, 179, 115,  12,  40, 169, 105, 253, 176, 211,  59 },
    { 100, 180, 145, 122, 172,  97, 235, 129, 215, 149, 199,   8,  72,  26, 238,  44 },
    { 232,  31,  69,  11, 205,  58,  18, 193,  88,  60, 112, 221, 140,  86, 120, 153 },
    { 208, 130, 243, 160, 224, 110,  34, 248, 165,  24, 234, 184,  52, 198, 171,   6 },
    { 108, 188,  51,  89, 137, 186, 154,  78,  47, 134,  98, 157,  35, 249,  95,  63 },
    {  16,  75, 219,  39,   0,  67, 228, 121, 197, 240,   3,  74, 127,  20, 227, 143 },
    { 246, 175, 119, 200, 251, 103, 146,  14, 209, 174, 109, 218, 192,  82, 203, 163 },
    {  29,  93, 150,  22, 166, 182,  55,  30,  90,  64,  42, 141, 168,  57, 117,  46 },
    { 216, 233,  61, 128,  81, 237, 217, 118, 159, 255, 185,  27, 242, 102,   4, 133 },
    {  73, 191,   9, 210,  43,  96,   7, 136, 231,  80,  10, 124, 225, 207, 155, 183 }
  };


  (void)data;
  (void)device_uri;
  (void)device_id;
  (void)driver_attrs;


  // Set dither arrays...
  for (i = 0; i < 16; i ++)
  {
    // Apply gamma correction to dither array...
    for (j = 0; j < 16; j ++)
      driver_data->gdither[i][j] = 255 - (int)(255.0 * pow(1.0 - dither[i][j] / 255.0, 0.4545));
  }

  // Same dither array for photo as well...
  memcpy(driver_data->pdither, driver_data->gdither, sizeof(driver_data->pdither));

  /* Set callbacks */
  driver_data->printfile_cb  = pcl_print;
  driver_data->rendjob_cb    = pcl_rendjob;
  driver_data->rendpage_cb   = pcl_rendpage;
  driver_data->rstartjob_cb  = pcl_rstartjob;
  driver_data->rstartpage_cb = pcl_rstartpage;
  driver_data->rwriteline_cb = pcl_rwriteline;
  driver_data->status_cb     = pcl_status;
  driver_data->has_supplies  = true;

  /* Native format */
  driver_data->format = "application/vnd.hp-pcl";

  /* Default orientation and quality */
  driver_data->orient_default  = IPP_ORIENT_NONE;
  driver_data->quality_default = IPP_QUALITY_NORMAL;

  if (!strcmp(driver_name, "hp_deskjet"))
  {
    /* Make and model name */
    papplCopyString(driver_data->make_and_model, "HP DeskJet", sizeof(driver_data->make_and_model));

    /* Icons */
    driver_data->icons[0].data    = hp_deskjet_sm_png;
    driver_data->icons[0].datalen = sizeof(hp_deskjet_sm_png);

    driver_data->icons[1].data    = hp_deskjet_md_png;
    driver_data->icons[1].datalen = sizeof(hp_deskjet_md_png);

    driver_data->icons[2].data    = hp_deskjet_lg_png;
    driver_data->icons[2].datalen = sizeof(hp_deskjet_lg_png);

    /* Pages-per-minute for monochrome and color */
    driver_data->ppm       = 8;
    driver_data->ppm_color = 2;

    /* Three resolutions - 150dpi, 300dpi (default), and 600dpi */
    driver_data->num_resolution  = 3;
    driver_data->x_resolution[0] = 150;
    driver_data->y_resolution[0] = 150;
    driver_data->x_resolution[1] = 300;
    driver_data->y_resolution[1] = 300;
    driver_data->x_resolution[2] = 600;
    driver_data->y_resolution[2] = 600;
    driver_data->x_default = driver_data->y_default = 300;

    /* Four color spaces - black (1-bit and 8-bit), grayscale, and sRGB */
    driver_data->raster_types = PAPPL_PWG_RASTER_TYPE_BLACK_1 | PAPPL_PWG_RASTER_TYPE_BLACK_8 | PAPPL_PWG_RASTER_TYPE_SGRAY_8 | PAPPL_PWG_RASTER_TYPE_SRGB_8;

    /* Color modes: auto (default), monochrome, and color */
    driver_data->color_supported = PAPPL_COLOR_MODE_AUTO | PAPPL_COLOR_MODE_AUTO_MONOCHROME | PAPPL_COLOR_MODE_COLOR | PAPPL_COLOR_MODE_MONOCHROME;
    driver_data->color_default   = PAPPL_COLOR_MODE_AUTO;

    /* Media sizes with 1/4" left/right and 1/2" top/bottom margins*/
    driver_data->num_media = (int)(sizeof(pcl_hp_deskjet_media) / sizeof(pcl_hp_deskjet_media[0]));
    memcpy(driver_data->media, pcl_hp_deskjet_media, sizeof(pcl_hp_deskjet_media));

    driver_data->left_right = 635;	 // 1/4" left and right
    driver_data->bottom_top = 1270;	 // 1/2" top and bottom

    /* 1-sided printing only */
    driver_data->sides_supported = PAPPL_SIDES_ONE_SIDED;
    driver_data->sides_default   = PAPPL_SIDES_ONE_SIDED;

    /* Three paper trays (MSN names) */
    driver_data->num_source = 3;
    driver_data->source[0]  = "tray-1";
    driver_data->source[1]  = "manual";
    driver_data->source[2]  = "envelope";

    /* Media types (MSN names) */
    driver_data->num_type = 6;
    driver_data->type[0] = "stationery";
    driver_data->type[1] = "stationery-inkjet";
    driver_data->type[2] = "stationery-letterhead";
    driver_data->type[3] = "envelope";
    driver_data->type[4] = "transparency";
    driver_data->type[5] = "photographic";
  }
  else if (!strcmp(driver_name, "hp_generic"))
  {
    /* Make and model name */
    papplCopyString(driver_data->make_and_model, "Generic PCL Laser Printer", sizeof(driver_data->make_and_model));

    /* Icons */
    driver_data->icons[0].data    = hp_generic_sm_png;
    driver_data->icons[0].datalen = sizeof(hp_generic_sm_png);

    driver_data->icons[1].data    = hp_generic_md_png;
    driver_data->icons[1].datalen = sizeof(hp_generic_md_png);

    driver_data->icons[2].data    = hp_generic_lg_png;
    driver_data->icons[2].datalen = sizeof(hp_generic_lg_png);

    /* Pages-per-minute for monochrome and color */
    driver_data->ppm = 10;

    /* Two resolutions - 300dpi (default) and 600dpi */
    driver_data->num_resolution  = 2;
    driver_data->x_resolution[0] = 300;
    driver_data->y_resolution[0] = 300;
    driver_data->x_resolution[1] = 600;
    driver_data->y_resolution[1] = 600;
    driver_data->x_default = driver_data->y_default = 300;

    driver_data->raster_types = PAPPL_PWG_RASTER_TYPE_BLACK_1 | PAPPL_PWG_RASTER_TYPE_BLACK_8 | PAPPL_PWG_RASTER_TYPE_SGRAY_8;

    driver_data->color_supported = PAPPL_COLOR_MODE_MONOCHROME;
    driver_data->color_default   = PAPPL_COLOR_MODE_MONOCHROME;

    driver_data->num_media = (int)(sizeof(pcl_generic_pcl_media) / sizeof(pcl_generic_pcl_media[0]));
    memcpy(driver_data->media, pcl_generic_pcl_media, sizeof(pcl_generic_pcl_media));

    driver_data->sides_supported = PAPPL_SIDES_ONE_SIDED | PAPPL_SIDES_TWO_SIDED_LONG_EDGE | PAPPL_SIDES_TWO_SIDED_SHORT_EDGE;
    driver_data->sides_default   = PAPPL_SIDES_ONE_SIDED;

    driver_data->num_source = 7;
    driver_data->source[0]  = "default";
    driver_data->source[1]  = "tray-1";
    driver_data->source[2]  = "tray-2";
    driver_data->source[3]  = "tray-3";
    driver_data->source[4]  = "tray-4";
    driver_data->source[5]  = "manual";
    driver_data->source[6]  = "envelope";

    /* Media types (MSN names) */
    driver_data->num_type = 4;
    driver_data->type[0] = "stationery";
    driver_data->type[1] = "stationery-letterhead";
    driver_data->type[2] = "envelope";
    driver_data->type[3] = "transparency";

    driver_data->left_right = 635;	// 1/4" left and right
    driver_data->bottom_top = 423;	// 1/6" top and bottom

    for (i = 0; i < driver_data->num_source; i ++)
    {
      if (strcmp(driver_data->source[i], "envelope"))
        snprintf(driver_data->media_ready[i].size_name, sizeof(driver_data->media_ready[i].size_name), "na_letter_8.5x11in");
      else
        snprintf(driver_data->media_ready[i].size_name, sizeof(driver_data->media_ready[i].size_name), "env_10_4.125x9.5in");
    }
  }
  else if (!strcmp(driver_name, "hp_laserjet"))
  {
    /* Make and model name */
    papplCopyString(driver_data->make_and_model, "HP LaserJet", sizeof(driver_data->make_and_model));

    /* Icons */
    driver_data->icons[0].data    = hp_laserjet_sm_png;
    driver_data->icons[0].datalen = sizeof(hp_laserjet_sm_png);

    driver_data->icons[1].data    = hp_laserjet_md_png;
    driver_data->icons[1].datalen = sizeof(hp_laserjet_md_png);

    driver_data->icons[2].data    = hp_laserjet_lg_png;
    driver_data->icons[2].datalen = sizeof(hp_laserjet_lg_png);

    /* Pages-per-minute for monochrome and color */
    driver_data->ppm = 10;

    /* Three resolutions - 150dpi, 300dpi (default), and 600dpi */
    driver_data->num_resolution  = 3;
    driver_data->x_resolution[0] = 150;
    driver_data->y_resolution[0] = 150;
    driver_data->x_resolution[1] = 300;
    driver_data->y_resolution[1] = 300;
    driver_data->x_resolution[2] = 600;
    driver_data->y_resolution[2] = 600;
    driver_data->x_default = driver_data->y_default = 300;

    driver_data->raster_types = PAPPL_PWG_RASTER_TYPE_BLACK_1 | PAPPL_PWG_RASTER_TYPE_BLACK_8 | PAPPL_PWG_RASTER_TYPE_SGRAY_8;

    driver_data->color_supported = PAPPL_COLOR_MODE_MONOCHROME;
    driver_data->color_default   = PAPPL_COLOR_MODE_MONOCHROME;

    driver_data->num_media = (int)(sizeof(pcl_hp_laserjet_media) / sizeof(pcl_hp_laserjet_media[0]));
    memcpy(driver_data->media, pcl_hp_laserjet_media, sizeof(pcl_hp_laserjet_media));

    driver_data->sides_supported = PAPPL_SIDES_ONE_SIDED | PAPPL_SIDES_TWO_SIDED_LONG_EDGE | PAPPL_SIDES_TWO_SIDED_SHORT_EDGE;
    driver_data->sides_default   = PAPPL_SIDES_ONE_SIDED;

    driver_data->num_source = 7;
    driver_data->source[0]  = "default";
    driver_data->source[1]  = "tray-1";
    driver_data->source[2]  = "tray-2";
    driver_data->source[3]  = "tray-3";
    driver_data->source[4]  = "tray-4";
    driver_data->source[5]  = "manual";
    driver_data->source[6]  = "envelope";

    /* Media types (MSN names) */
    driver_data->num_type = 4;
    driver_data->type[0] = "stationery";
    driver_data->type[1] = "stationery-letterhead";
    driver_data->type[2] = "envelope";
    driver_data->type[3] = "transparency";

    driver_data->left_right = 635;	 // 1/4" left and right
    driver_data->bottom_top = 1270;	 // 1/2" top and bottom

    for (i = 0; i < driver_data->num_source; i ++)
    {
      if (strcmp(driver_data->source[i], "envelope"))
        snprintf(driver_data->media_ready[i].size_name, sizeof(driver_data->media_ready[i].size_name), "na_letter_8.5x11in");
      else
        snprintf(driver_data->media_ready[i].size_name, sizeof(driver_data->media_ready[i].size_name), "env_10_4.125x9.5in");
    }
  }
  else
  {
    papplLog(system, PAPPL_LOGLEVEL_ERROR, "No dimension information in driver name '%s'.", driver_name);
    return (false);
  }

  // Fill out ready and default media (default == ready media from the first source)
  for (i = 0; i < driver_data->num_source; i ++)
  {
    pwg_media_t *pwg;			/* Media size information */

    /* Use US Letter for regular trays, #10 envelope for the envelope tray */
    if (!strcmp(driver_data->source[i], "envelope"))
      papplCopyString(driver_data->media_ready[i].size_name, "na_number-10_4.125x9.5in", sizeof(driver_data->media_ready[i].size_name));
    else
      papplCopyString(driver_data->media_ready[i].size_name, "na_letter_8.5x11in", sizeof(driver_data->media_ready[i].size_name));

    /* Set margin and size information */
    if ((pwg = pwgMediaForPWG(driver_data->media_ready[i].size_name)) != NULL)
    {
      driver_data->media_ready[i].bottom_margin = driver_data->bottom_top;
      driver_data->media_ready[i].left_margin   = driver_data->left_right;
      driver_data->media_ready[i].right_margin  = driver_data->left_right;
      driver_data->media_ready[i].size_width    = pwg->width;
      driver_data->media_ready[i].size_length   = pwg->length;
      driver_data->media_ready[i].top_margin    = driver_data->bottom_top;
      papplCopyString(driver_data->media_ready[i].source, driver_data->source[i], sizeof(driver_data->media_ready[i].source));
      papplCopyString(driver_data->media_ready[i].type, driver_data->type[0],  sizeof(driver_data->media_ready[i].type));
    }
  }

  driver_data->media_default = driver_data->media_ready[0];

  return (true);
}


//
// 'pcl_compress_data()' - Compress a line of graphics.
//

static void
pcl_compress_data(
    pappl_job_t    *job,		// I - Job object
    pappl_device_t *device,		// I - Device
    unsigned char  *line,		// I - Data to compress
    unsigned       length,		// I - Number of bytes
    unsigned       plane,		// I - Color plane
    unsigned       type)		// I - Type of compression
{
  unsigned char	*line_ptr,		// Current byte pointer
		*line_end,		// End-of-line byte pointer
		*comp_ptr,		// Pointer into compression buffer
		*start;			// Start of compression sequence
  unsigned	count;			// Count of bytes for output
  pcl_t		*pcl = (pcl_t *)papplJobGetData(job);
					// Job data


  switch (type)
  {
    default : // No compression...
	line_ptr = line;
	line_end = line + length;
	break;

    case 1 :  // Do run-length encoding...
	line_end = line + length;
	for (line_ptr = line, comp_ptr = pcl->comp_buffer; line_ptr < line_end; comp_ptr += 2, line_ptr += count)
	{
	  count = 1;
	  while ((line_ptr + count) < line_end && line_ptr[0] == line_ptr[count] && count < 256)
	    count ++;

	  comp_ptr[0] = (unsigned char)(count - 1);
	  comp_ptr[1] = line_ptr[0];
	}

	line_ptr = pcl->comp_buffer;
	line_end = comp_ptr;
	break;

    case 2 :  // Do TIFF pack-bits encoding...
	line_ptr = line;
	line_end = line + length;
	comp_ptr = pcl->comp_buffer;

	while (line_ptr < line_end)
	{
	  if ((line_ptr + 1) >= line_end)
	  {
	    // Single byte on the end...
	    *comp_ptr++ = 0x00;
	    *comp_ptr++ = *line_ptr++;
	  }
	  else if (line_ptr[0] == line_ptr[1])
	  {
	    // Repeated sequence...
	    line_ptr ++;
	    count = 2;

	    while (line_ptr < (line_end - 1) && line_ptr[0] == line_ptr[1] && count < 127)
	    {
	      line_ptr ++;
	      count ++;
	    }

	    *comp_ptr++ = (unsigned char)(257 - count);
	    *comp_ptr++ = *line_ptr++;
	  }
	  else
	  {
	    // Non-repeated sequence...
	    start    = line_ptr;
	    line_ptr ++;
	    count    = 1;

	    while (line_ptr < (line_end - 1) && line_ptr[0] != line_ptr[1] && count < 127)
	    {
	      line_ptr ++;
	      count ++;
	    }

	    *comp_ptr++ = (unsigned char)(count - 1);

	    memcpy(comp_ptr, start, count);
	    comp_ptr += count;
	  }
	}

	line_ptr = pcl->comp_buffer;
	line_end = comp_ptr;
	break;
  }

  // Set the length of the data and write a raster plane...
  papplDevicePrintf(device, "\033*b%d%c", (int)(line_end - line_ptr), plane);
  papplDeviceWrite(device, line_ptr, (size_t)(line_end - line_ptr));
}


//
// 'pcl_print()' - Print file.
//

static bool				// O - `true` on success, `false` on failure
pcl_print(
    pappl_job_t        *job,		// I - Job
    pappl_pr_options_t *options,	// I - Options
    pappl_device_t     *device)		// I - Device
{
  int		fd;			// Job file
  ssize_t	bytes;			// Bytes read/written
  char		buffer[65536];		// Read/write buffer


  papplLogJob(job, PAPPL_LOGLEVEL_DEBUG, "Printing raw file...");

  papplJobSetImpressions(job, 1);

  fd = open(papplJobGetFilename(job), O_RDONLY);

  while ((bytes = read(fd, buffer, sizeof(buffer))) > 0)
  {
    if (papplDeviceWrite(device, buffer, (size_t)bytes) < 0)
    {
      papplLogJob(job, PAPPL_LOGLEVEL_ERROR, "Unable to send %d bytes to printer.", (int)bytes);
      close(fd);
      return (false);
    }
  }

  close(fd);

  papplJobSetImpressionsCompleted(job, 1);

  return (true);
}


//
// 'pcl_rendjob()' - End a job.
//

static bool				// O - `true` on success, `false` on failure
pcl_rendjob(
    pappl_job_t        *job,		// I - Job
    pappl_pr_options_t *options,	// I - Options
    pappl_device_t     *device)		// I - Device
{
  pcl_t	*pcl = (pcl_t *)papplJobGetData(job);
					// Job data


  papplLogJob(job, PAPPL_LOGLEVEL_DEBUG, "Ending job...");

  (void)options;

  free(pcl);
  papplJobSetData(job, NULL);

  return (true);
}


//
// 'pcl_rendpage()' - End a page.
//

static bool				// O - `true` on success, `false` on failure
pcl_rendpage(
    pappl_job_t        *job,		// I - Job
    pappl_pr_options_t *options,	// I - Job options
    pappl_device_t     *device,		// I - Device
    unsigned           page)		// I - Page number
{
  pcl_t	*pcl = (pcl_t *)papplJobGetData(job);
					// Job data


  papplLogJob(job, PAPPL_LOGLEVEL_DEBUG, "Ending page %u...", page);

  // Eject the current page...
  if (pcl->num_planes > 1)
  {
    papplDevicePuts(device, "\033*rC"); // End color GFX

    if (!(options->header.Duplex && (page & 1)))
      papplDevicePuts(device, "\033&l0H");
					// Eject current page
  }
  else
  {
    papplDevicePuts(device, "\033*r0B");// End GFX

    if (!(options->header.Duplex && (page & 1)))
      papplDevicePuts(device, "\014");  // Eject current page
  }

  papplDeviceFlush(device);

  // Free memory...
  free(pcl->planes[0]);

  if (pcl->comp_buffer)
    free(pcl->comp_buffer);

  return (true);
}


//
// 'pcl_rstartjob()' - Start a job.
//

static bool				// O - `true` on success, `false` on failure
pcl_rstartjob(
    pappl_job_t        *job,		// I - Job
    pappl_pr_options_t *options,	// I - Job options
    pappl_device_t     *device)		// I - Device
{
  pcl_t	*pcl = (pcl_t *)calloc(1, sizeof(pcl_t));
					// Job data

  papplLogJob(job, PAPPL_LOGLEVEL_DEBUG, "Starting job...");

  (void)options;

  pcl->is_deskjet = !strcmp(papplPrinterGetDriverName(papplJobGetPrinter(job)), "hp_deskjet");

  papplJobSetData(job, pcl);

  papplDevicePuts(device, "\033E");	// PCL reset sequence

  return (true);
}


//
// 'pcl_rstartpage()' - Start a page.
//

static bool				// O - `true` on success, `false` on failure
pcl_rstartpage(
    pappl_job_t        *job,		// I - Job
    pappl_pr_options_t *options,	// I - Job options
    pappl_device_t     *device,		// I - Device
    unsigned           page)		// I - Page number
{
  unsigned	plane,			// Looping var
		length;			// Bytes to write
  cups_page_header2_t *header = &(options->header);
					// Page header
  pcl_t		*pcl = (pcl_t *)papplJobGetData(job);
					// Job data


  papplLogJob(job, PAPPL_LOGLEVEL_DEBUG, "Starting page %u...", page);
  int i;
  for (i = 0; i < 16; i ++)
    papplLogJob(job, PAPPL_LOGLEVEL_DEBUG, "DITHER %d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d", options->dither[i][0], options->dither[i][1], options->dither[i][2], options->dither[i][3], options->dither[i][4], options->dither[i][5], options->dither[i][6], options->dither[i][7], options->dither[i][8], options->dither[i][9], options->dither[i][10], options->dither[i][11], options->dither[i][12], options->dither[i][13], options->dither[i][14], options->dither[i][15]);

  // Setup size based on margins...
  pcl->width    = options->printer_resolution[0] * (options->media.size_width - options->media.left_margin - options->media.right_margin) / 2540;
  pcl->height   = options->printer_resolution[1] * (options->media.size_length - options->media.top_margin - options->media.bottom_margin) / 2540;
  pcl->linesize = (pcl->width + 7) / 8;
  pcl->xstart   = options->printer_resolution[0] * options->media.left_margin / 2540;
  pcl->xend     = pcl->xstart + pcl->width;
  pcl->ystart   = options->printer_resolution[1] * options->media.top_margin / 2540;
  pcl->yend     = pcl->ystart + pcl->height;

  // Setup printer/job attributes...
  if ((!header->Duplex || (page & 1)) && header->MediaPosition)
    papplDevicePrintf(device, "\033&l%dH", header->MediaPosition);
					// Set media position

  if (header->Duplex && pcl->is_deskjet)
  {
    // Handle duplexing on DeskJet printers...
    papplDevicePuts(device, "\033&l-2H");	// Load media

    if (page & 1)
      papplDevicePuts(device, "\033&l2S");	// Set duplex mode
  }

  if (!header->Duplex || (page & 1))
  {
    // Set the media size...
    papplDevicePuts(device, "\033&l6D\033&k12H");
					// Set 6 LPI, 10 CPI
    papplDevicePuts(device, "\033&l0O");// Set portrait orientation

    // Set page size
    switch (header->PageSize[1])
    {
      case 540 : // Monarch Envelope
          papplDevicePuts(device, "\033&l80A");
	  break;

      case 595 : // A5
          papplDevicePuts(device, "\033&l25A");
	  break;

      case 624 : // DL Envelope
          papplDevicePuts(device, "\033&l90A");
	  break;

      case 649 : // C5 Envelope
          papplDevicePuts(device, "\033&l91A");
	  break;

      case 684 : // COM-10 Envelope
          papplDevicePuts(device, "\033&l81A");
	  break;

      case 709 : // B5 Envelope
          papplDevicePuts(device, "\033&l100A");
	  break;

      case 756 : // Executive
          papplDevicePuts(device, "\033&l1A");
	  break;

      case 792 : // Letter
          papplDevicePuts(device, "\033&l2A");
	  break;

      case 842 : // A4
          papplDevicePuts(device, "\033&l26A");
	  break;

      case 1008 : // Legal
          papplDevicePuts(device, "\033&l3A");
	  break;

      case 1191 : // A3
          papplDevicePuts(device, "\033&l27A");
	  break;

      case 1224 : // Tabloid
          papplDevicePuts(device, "\033&l6A");
	  break;
    }

    papplDevicePrintf(device, "\033&l%dP", header->PageSize[1] / 12);
					// Set page length
    papplDevicePuts(device, "\033&l0E");// Set top margin to 0

    // Set other job options...
    papplDevicePrintf(device, "\033&l%dX", header->NumCopies);
					// Set number of copies

    // TODO: Fix me
    if (header->cupsMediaType)
      papplDevicePrintf(device, "\033&l%dM", header->cupsMediaType);
					// Set media type

    int mode = header->Duplex ? 1 + header->Tumble != 0 : 0;

    papplDevicePrintf(device, "\033&l%dS", mode);
					// Set duplex mode
    papplDevicePuts(device, "\033&l0L");// Turn off perforation skip
  }
  else
    papplDevicePuts(device, "\033&a2G");// Set back side

  // Set graphics mode...
  papplDevicePrintf(device, "\033*t%uR", header->HWResolution[0]);
					// Set resolution

  if (header->cupsColorSpace == CUPS_CSPACE_SRGB)
  {
    pcl->num_planes = 4;
    papplDevicePuts(device, "\033*r-4U");
					// Set KCMY graphics
  }
  else
    pcl->num_planes = 1;		// Black&white graphics

  // Set size and position of graphics...
  papplDevicePrintf(device, "\033*r%uS", pcl->width);
					// Set width
  papplDevicePrintf(device, "\033*r%uT", pcl->height);
					// Set height

  papplDevicePuts(device, "\033&a0H");	// Set horizontal position

  papplDevicePrintf(device, "\033&a%.0fV", 720.0 * options->media.top_margin / 2540.0);
					// Set vertical position

  papplDevicePuts(device, "\033*r1A");	// Start graphics

  pcl->compression = 2;			// Packbits compression
  papplDevicePrintf(device, "\033*b%uM", pcl->compression);
					// Set compression

  pcl->feed = 0;			// No blank lines yet

  // Allocate memory for a line of graphics...
  if ((pcl->planes[0] = malloc(pcl->linesize * pcl->num_planes)) == NULL)
  {
    papplLogJob(job, PAPPL_LOGLEVEL_ERROR, "Malloc failure...");
    return (false);
  }

  for (plane = 1; plane < pcl->num_planes; plane ++)
    pcl->planes[plane] = pcl->planes[0] + plane * pcl->linesize;

  if (pcl->compression)
    pcl->comp_buffer = malloc(pcl->linesize * 2 + 2);
  else
    pcl->comp_buffer = NULL;

  return (true);
}


//
// 'pcl_rwriteline()' - Write a line.
//

static bool				// O - `true` on success, `false` on failure
pcl_rwriteline(
    pappl_job_t         *job,		// I - Job
    pappl_pr_options_t  *options,	// I - Job options
    pappl_device_t      *device,	// I - Device
    unsigned            y,		// I - Line number
    const unsigned char *pixels)	// I - Line
{
  cups_page_header2_t	*header = &(options->header);
					// Page header
  pcl_t			*pcl = (pcl_t *)papplJobGetData(job);
					// Job data
  unsigned		plane,		// Current plane
			x;		// Current column
  const unsigned char	*pixptr;	// Pixel pointer in line
  unsigned char		bit,		// Current plane data
			*cptr,		// Pointer into c-plane
			*mptr,		// Pointer into m-plane
			*yptr,		// Pointer into y-plane
			*kptr,		// Pointer into k-plane
			byte;		// Byte in line
  const unsigned char	*dither;	// Dither line


  // Skip top and bottom margin areas...
  if (y < pcl->ystart || y >= pcl->yend)
    return (true);

  if (!(y & 127))
    papplLogJob(job, PAPPL_LOGLEVEL_DEBUG, "Printing line %u (%u%%)", y, 100 * (y - pcl->ystart) / pcl->height);

  byte = options->header.cupsColorSpace == CUPS_CSPACE_K ? 0 : 255;

  if (*pixels != byte || memcmp(pixels, pixels + 1, header->cupsBytesPerLine - 1))
  {
    // Output whitespace as needed...
    if (pcl->feed > 0)
    {
      papplDevicePrintf(device, "\033*b%dY", pcl->feed);
      pcl->feed = 0;
    }

    // Write bitmap data as needed...
    dither = options->dither[y & 15];

    if (pcl->num_planes > 1)
    {
      // RGB
      memset(pcl->planes[0], 0, pcl->num_planes * pcl->linesize);

      for (x = pcl->xstart, cptr = pcl->planes[1], mptr = pcl->planes[2], yptr = pcl->planes[3], kptr = pcl->planes[0], pixptr = pixels + 3 * pcl->xstart, bit = 128; x < pcl->xend; x ++)
      {
        if (*pixptr ++ < dither[x & 15])
          *cptr |= bit;
        if (*pixptr ++ < dither[x & 15])
          *mptr |= bit;
        if (*pixptr ++ < dither[x & 15])
          *yptr |= bit;

        if (bit == 1)
        {
          *kptr   = *cptr & *mptr & *yptr;
          byte    = ~*kptr;
          *cptr++ &= byte;
          *mptr++ &= byte;
          *yptr++ &= byte;
          kptr ++;
          bit = 128;
        }
        else
          bit /= 2;
      }
    }
    else if (header->cupsBitsPerPixel == 8)
    {
      memset(pcl->planes[0], 0, pcl->linesize);

      if (header->cupsColorSpace == CUPS_CSPACE_K)
      {
        // 8 bit black
        for (x = pcl->xstart, kptr = pcl->planes[0], pixptr = pixels + pcl->xstart, bit = 128, byte = 0; x < pcl->xend; x ++, pixptr ++)
        {
          if (*pixptr >= dither[x & 15])
            byte |= bit;

          if (bit == 1)
          {
            *kptr++ = byte;
            byte    = 0;
            bit     = 128;
          }
          else
            bit /= 2;
        }

        if (bit < 128)
          *kptr = byte;
      }
      else
      {
        // 8 bit gray
        for (x = pcl->xstart, kptr = pcl->planes[0], pixptr = pixels + pcl->xstart, bit = 128, byte = 0; x < pcl->xend; x ++, pixptr ++)
        {
          if (*pixptr < dither[x & 15])
            byte |= bit;

          if (bit == 1)
          {
            *kptr++ = byte;
            byte    = 0;
            bit     = 128;
          }
          else
            bit /= 2;
        }

        if (bit < 128)
          *kptr = byte;
      }
    }
    else
    {
      // 1-bit B&W
      memcpy(pcl->planes[0], pixels + pcl->xstart / 8, pcl->linesize);
    }

    for (plane = 0; plane < pcl->num_planes; plane ++)
    {
      pcl_compress_data(job, device, pcl->planes[plane], pcl->linesize, plane < (pcl->num_planes - 1) ? 'V' : 'W', pcl->compression);
    }

    papplDeviceFlush(device);
  }
  else
    pcl->feed ++;

  return (true);
}


//
// 'pcl_status()' - Get printer status.
//

static bool				// O - `true` on success, `false` on failure
pcl_status(
    pappl_printer_t *printer)		// I - Printer
{
  pappl_device_t	*device;	// Printer device
  int			num_supply;	// Number of supplies
  pappl_supply_t	supply[32];	// Printer supply information


  papplLogPrinter(printer, PAPPL_LOGLEVEL_DEBUG, "Checking status...");

  // First try to query the supply levels via SNMP...
  if ((device = papplPrinterOpenDevice(printer)) != NULL)
  {
    if ((num_supply = papplDeviceGetSupplies(device, (int)(sizeof(supply) / sizeof(supply[0])), supply)) > 0)
      papplPrinterSetSupplies(printer, num_supply, supply);

    papplPrinterCloseDevice(printer);

    if (num_supply > 0)
      return (true);
  }

  // Otherwise make sure we have some dummy data to make clients happy...
  if (!strcmp(papplPrinterGetDriverName(printer), "hp_deskjet"))
  {
    static pappl_supply_t inkjet[5] =	// Dummy inkjet supply level data
    {
      { PAPPL_SUPPLY_COLOR_CYAN,     "Cyan Ink",       true, 20, PAPPL_SUPPLY_TYPE_INK },
      { PAPPL_SUPPLY_COLOR_MAGENTA,  "Magenta Ink",    true, 40, PAPPL_SUPPLY_TYPE_INK },
      { PAPPL_SUPPLY_COLOR_YELLOW,   "Yellow Ink",     true, 60, PAPPL_SUPPLY_TYPE_INK },
      { PAPPL_SUPPLY_COLOR_BLACK,    "Black Ink",      true, 80, PAPPL_SUPPLY_TYPE_INK },
      { PAPPL_SUPPLY_COLOR_NO_COLOR, "Waste Ink Tank", true, 50, PAPPL_SUPPLY_TYPE_WASTE_INK }
    };

    if (papplPrinterGetSupplies(printer, 0, supply) == 0)
      papplPrinterSetSupplies(printer, (int)(sizeof(inkjet) / sizeof(inkjet[0])), inkjet);
  }
  else
  {
    static pappl_supply_t laser[2] =	// Dummy laser supply level data
    {
      { PAPPL_SUPPLY_COLOR_BLACK,    "Black Toner", true, 80, PAPPL_SUPPLY_TYPE_TONER },
      { PAPPL_SUPPLY_COLOR_NO_COLOR, "Waste Toner", true, 40, PAPPL_SUPPLY_TYPE_WASTE_TONER }
    };

    if (papplPrinterGetSupplies(printer, 0, supply) == 0)
      papplPrinterSetSupplies(printer, (int)(sizeof(laser) / sizeof(laser[0])), laser);
  }

  return (true);
}
