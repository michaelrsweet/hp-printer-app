//
// Program to decode a PCL 6/PCL-XL data stream.
//
// Copyright © 2022-2024 by Michael R Sweet
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>


//
// Constants...
//

enum pcl6_attr
{
  PCL6_ATTR_COLOR_SPACE = 3,
  PCL6_ATTR_MEDIA_SIZE = 37,
  PCL6_ATTR_MEDIA_SOURCE = 38,
  PCL6_ATTR_MEDIA_TYPE = 39,
  PCL6_ATTR_ORIENTATION = 40,
  PCL6_ATTR_SIMPLEX_PAGE_MODE = 52,
  PCL6_ATTR_DUPLEX_PAGE_MODE = 53,
  PCL6_ATTR_DUPLEX_PAGE_SIDE = 54,
  PCL6_ATTR_POINT = 76,
  PCL6_ATTR_COLOR_DEPTH = 98,
  PCL6_ATTR_BLOCK_HEIGHT = 99,
  PCL6_ATTR_COLOR_MAPPING = 100,
  PCL6_ATTR_COMPRESS_MODE = 101,
  PCL6_ATTR_DESTINATION_BOX = 102,
  PCL6_ATTR_DESTINATION_SIZE = 103,
  PCL6_ATTR_SOURCE_HEIGHT = 107,
  PCL6_ATTR_SOURCE_WIDTH = 108,
  PCL6_ATTR_START_LINE = 109,
  PCL6_ATTR_PAD_BYTES_MULTIPLE = 110,
  PCL6_ATTR_BLOCK_BYTE_LENGTH = 111,
  PCL6_ATTR_DATA_ORG = 130,
  PCL6_ATTR_MEASURE = 134,
  PCL6_ATTR_SOURCE_TYPE = 136,
  PCL6_ATTR_UNITS_PER_MEASURE = 137,
  PCL6_ATTR_ERROR_REPORT = 143
};

enum pcl6_cmd
{
  PCL6_CMD_BEGIN_SESSION = 0x41,
  PCL6_CMD_END_SESSION = 0x42,
  PCL6_CMD_BEGIN_PAGE = 0x43,
  PCL6_CMD_END_PAGE = 0x44,
  PCL6_CMD_OPEN_DATA_SOURCE = 0x48,
  PCL6_CMD_CLOSE_DATA_SOURCE = 0x49,
  PCL6_CMD_SET_COLOR_SPACE = 0x6a,
  PCL6_CMD_SET_CURSOR = 0x6b,
  PCL6_CMD_BEGIN_IMAGE = 0xb0,
  PCL6_CMD_READ_IMAGE = 0xb1,
  PCL6_CMD_END_IMAGE = 0xb2
};

enum pcl6_enc
{
  PCL6_ENC_UBYTE = 0xc0,
  PCL6_ENC_UINT16 = 0xc1,
  PCL6_ENC_UINT32 = 0xc2,
  PCL6_ENC_SINT16 = 0xc3,
  PCL6_ENC_SINT32 = 0xc4,
  PCL6_ENC_REAL32 = 0xc5,

  PCL6_ENC_UBYTE_ARRAY = 0xc8,
  PCL6_ENC_UINT16_ARRAY = 0xc9,
  PCL6_ENC_UINT32_ARRAY = 0xca,
  PCL6_ENC_SINT16_ARRAY = 0xcb,
  PCL6_ENC_SINT32_ARRAY = 0xcc,
  PCL6_ENC_REAL32_ARRAY = 0xcd,

  PCL6_ENC_UBYTE_XY = 0xd0,
  PCL6_ENC_UINT16_XY = 0xd1,
  PCL6_ENC_UINT32_XY = 0xd2,
  PCL6_ENC_SINT16_XY = 0xd3,
  PCL6_ENC_SINT32_XY = 0xd4,
  PCL6_ENC_REAL32_XY = 0xd5,

  PCL6_ENC_UBYTE_BOX = 0xe0,
  PCL6_ENC_UINT16_BOX = 0xe1,
  PCL6_ENC_UINT32_BOX = 0xe2,
  PCL6_ENC_SINT16_BOX = 0xe3,
  PCL6_ENC_SINT32_BOX = 0xe4,
  PCL6_ENC_REAL32_BOX = 0xe5,

  PCL6_ENC_ATTR_UBYTE = 0xf8,
  PCL6_ENC_ATTR_UINT16 = 0xf9,
  PCL6_ENC_EMBEDDED_DATA = 0xfa,
  PCL6_ENC_EMBEDDED_DATA_BYTE = 0xfb
};


//
// Local globals...
//

static bool	big_endian = false;


//
// Local functions...
//

static const char	*attr_name(enum pcl6_attr attr);
static const char	*cmd_name(enum pcl6_cmd command);
static const char	*enc_name(enum pcl6_enc encoding);
static unsigned		read_uint16(FILE *fp);
static unsigned		read_uint32(FILE *fp);


//
// 'main()' - Main entry.
//

int					// O - Exit status
main(int  argc,				// I - Number of command-line arguments
     char *argv[])			// I - Command-line arguments
{
  FILE		*fp;			// File
  struct stat	finfo;			// File information
  long		pos;			// File position
  int		ch;			// Current character
  enum pcl6_enc	encoding;		// Value encoding
  enum pcl6_attr attr;			// Current attribute
  unsigned	number,			// Current number value
		xy[2],			// Current X,Y value
		length = 0;		// Current block length


  // Open file on command-line...
  if (argc != 2 || argv[1][0] == '-')
  {
    puts("Usage: decode-pcl6 FILENAME.pxl");
    return (1);
  }

  if ((fp = fopen(argv[1], "rb")) == NULL)
  {
    perror(argv[1]);
    return (1);
  }

  if (fstat(fileno(fp), &finfo))
  {
    perror(argv[1]);
    fclose(fp);
    return (1);
  }

  // Scan the file until end-of-file...
  while (!feof(fp))
  {
    pos = ftell(fp);

    if ((ch = getc(fp)) == EOF)
      break;

    printf("%08ld:", pos);

    if (ch < 0x41)
    {
      // Escape or other sequence, skip to end of line...
      if (ch == '(')
        big_endian = true;
      else if (ch == ')')
        big_endian = false;

      do
      {
        if (ch < ' ')
          printf(" %02X", (unsigned)ch);
        else
          printf(" %c", ch);

        if (ch == '\n')
          break;
      }
      while ((ch = getc(fp)) != EOF);

      putchar('\n');
    }
    else if (ch < 0xc0)
    {
      // Command
      printf(" ---> %s\n", cmd_name((enum pcl6_cmd)ch));

      if (ch == PCL6_CMD_READ_IMAGE && length == 0)
      {
        // Grab length after the command...
        if ((ch = getc(fp)) != PCL6_ENC_EMBEDDED_DATA && ch != PCL6_ENC_EMBEDDED_DATA_BYTE)
        {
          puts("        Read error - no embedded data length.");
          break;
        }
        else if (ch == PCL6_ENC_EMBEDDED_DATA_BYTE)
        {
          length = (unsigned)getc(fp);
        }
        else
        {
          length = read_uint32(fp);
        }
      }

      if (length > 0)
      {
        // Skip past data...
        if ((pos + 1 + length) > finfo.st_size)
        {
          printf("          Read error - need %u bytes, only have %u bytes.\n", length, (unsigned)(finfo.st_size - pos - 1));
          break;
        }

        fseek(fp, (long)length, SEEK_CUR);
        length = 0;
      }
    }
    else
    {
      // Value + attribute
      encoding = (enum pcl6_enc)ch;

      switch (encoding)
      {
        case PCL6_ENC_UBYTE :
            number = getc(fp);
            break;
        case PCL6_ENC_UINT16 :
            number = read_uint16(fp);
            break;
        case PCL6_ENC_UINT32 :
            number = read_uint32(fp);
            break;
        case PCL6_ENC_UBYTE_XY :
            xy[0] = getc(fp);
            xy[1] = getc(fp);
            break;
        case PCL6_ENC_UINT16_XY :
            xy[0] = read_uint16(fp);
            xy[1] = read_uint16(fp);
            break;
        case PCL6_ENC_UINT32_XY :
            xy[0] = read_uint32(fp);
            xy[1] = read_uint32(fp);
            break;
        default :
            fputs(" ???", stdout);
            break;
      }

      if ((ch = getc(fp)) != PCL6_ENC_ATTR_UBYTE && ch != PCL6_ENC_ATTR_UINT16)
      {
        puts(" Error, bad attribute/value pair.");
        break;
      }

      if (ch == PCL6_ENC_ATTR_UBYTE)
        attr = (enum pcl6_attr)getc(fp);
      else
        attr = (enum pcl6_attr)read_uint16(fp);

      switch (encoding)
      {
        case PCL6_ENC_UBYTE :
        case PCL6_ENC_UINT16 :
        case PCL6_ENC_UINT32 :
            printf(" %s %s %u\n", attr_name(attr), enc_name(encoding), number);
            if (attr == PCL6_ATTR_BLOCK_BYTE_LENGTH)
              length = number;
            break;
        case PCL6_ENC_UBYTE_XY :
        case PCL6_ENC_UINT16_XY :
        case PCL6_ENC_UINT32_XY :
            printf(" %s %s %u,%u\n", attr_name(attr), enc_name(encoding), xy[0], xy[1]);
            break;
        default :
            printf(" %s %s ???\n", attr_name(attr), enc_name(encoding));
            break;
      }
    }
  }

  // Close file and return...
  fclose(fp);

  return (0);
}


// 'attr_name()' - Return a string for the attribute
static const char *			// O - String
attr_name(enum pcl6_attr attr)		// I - Attribute
{
  static char	unknown[256];		// Unknown attribute string


  switch (attr)
  {
    case PCL6_ATTR_COLOR_SPACE :
	return ("ColorSpace");

    case PCL6_ATTR_MEDIA_SIZE :
	return ("MediaSize");

    case PCL6_ATTR_MEDIA_SOURCE :
	return ("MediaSource");

    case PCL6_ATTR_MEDIA_TYPE :
	return ("MediaType");

    case PCL6_ATTR_ORIENTATION :
	return ("Orientation");

    case PCL6_ATTR_SIMPLEX_PAGE_MODE :
	return ("SimplexPageMode");

    case PCL6_ATTR_DUPLEX_PAGE_MODE :
	return ("DuplexPageMode");

    case PCL6_ATTR_DUPLEX_PAGE_SIDE :
	return ("DuplexPageSide");

    case PCL6_ATTR_POINT :
	return ("Point");

    case PCL6_ATTR_COLOR_DEPTH :
	return ("ColorDepth");

    case PCL6_ATTR_BLOCK_HEIGHT :
	return ("BlockHeight");

    case PCL6_ATTR_COLOR_MAPPING :
	return ("ColorMapping");

    case PCL6_ATTR_COMPRESS_MODE :
	return ("CompressMode");

    case PCL6_ATTR_DESTINATION_BOX :
	return ("DestinationBox");

    case PCL6_ATTR_DESTINATION_SIZE :
	return ("DestinationSize");

    case PCL6_ATTR_SOURCE_HEIGHT :
	return ("SourceHeight");

    case PCL6_ATTR_SOURCE_WIDTH :
	return ("SourceWidth");

    case PCL6_ATTR_START_LINE :
	return ("StartLine");

    case PCL6_ATTR_PAD_BYTES_MULTIPLE :
	return ("PadBytesMultiple");

    case PCL6_ATTR_BLOCK_BYTE_LENGTH :
	return ("BlockByteLength");

    case PCL6_ATTR_DATA_ORG :
	return ("DataOrg");

    case PCL6_ATTR_MEASURE :
	return ("Measure");

    case PCL6_ATTR_SOURCE_TYPE :
	return ("SourceType");

    case PCL6_ATTR_UNITS_PER_MEASURE :
	return ("UnitsPerMeasure");

    case PCL6_ATTR_ERROR_REPORT :
	return ("ErrorReport");

    default :
        snprintf(unknown, sizeof(unknown), "Unknown-%d", (int)attr);
        return (unknown);
  }
}


static const char *			// O - String
cmd_name(enum pcl6_cmd command)		// I - Command
{
  static char	unknown[256];		// Unknown command string


  switch (command)
  {
    case PCL6_CMD_BEGIN_SESSION :
	return ("BeginSession");

    case PCL6_CMD_END_SESSION :
	return ("EndSession");

    case PCL6_CMD_BEGIN_PAGE :
	return ("BeginPage");

    case PCL6_CMD_END_PAGE :
	return ("EndPage");

    case PCL6_CMD_OPEN_DATA_SOURCE :
	return ("OpenDataSource");

    case PCL6_CMD_CLOSE_DATA_SOURCE :
	return ("CloseDataSource");

    case PCL6_CMD_SET_COLOR_SPACE :
	return ("SetColorSpace");

    case PCL6_CMD_SET_CURSOR :
	return ("SetCursor");

    case PCL6_CMD_BEGIN_IMAGE :
	return ("BeginImage");

    case PCL6_CMD_READ_IMAGE :
	return ("ReadImage");

    case PCL6_CMD_END_IMAGE :
	return ("EndImage");

    default :
        snprintf(unknown, sizeof(unknown), "Command-%02X", (unsigned)command);
        return (unknown);
  }
}


static const char *			// O - String
enc_name(enum pcl6_enc encoding)	// I - Encoding
{
  static char	unknown[256];		// Unknown command string


  switch (encoding)
  {
    case PCL6_ENC_UBYTE :
	return ("ubyte");

    case PCL6_ENC_UINT16 :
	return ("uint16");

    case PCL6_ENC_UINT32 :
	return ("uint32");

    case PCL6_ENC_SINT16 :
	return ("sint16");

    case PCL6_ENC_SINT32 :
	return ("sint32");

    case PCL6_ENC_REAL32 :
	return ("real32");

    case PCL6_ENC_UBYTE_ARRAY :
	return ("ubyte_array");

    case PCL6_ENC_UINT16_ARRAY :
	return ("uint16_array");

    case PCL6_ENC_UINT32_ARRAY :
	return ("uint32_array");

    case PCL6_ENC_SINT16_ARRAY :
	return ("sint16_array");

    case PCL6_ENC_SINT32_ARRAY :
	return ("sint32_array");

    case PCL6_ENC_REAL32_ARRAY :
	return ("real32_array");

    case PCL6_ENC_UBYTE_XY :
	return ("ubyte_xy");

    case PCL6_ENC_UINT16_XY :
	return ("uint16_xy");

    case PCL6_ENC_UINT32_XY :
	return ("uint32_xy");

    case PCL6_ENC_SINT16_XY :
	return ("sint16_xy");

    case PCL6_ENC_SINT32_XY :
	return ("sint32_xy");

    case PCL6_ENC_REAL32_XY :
	return ("real32_xy");

    case PCL6_ENC_UBYTE_BOX :
	return ("ubyte_box");

    case PCL6_ENC_UINT16_BOX :
	return ("uint16_box");

    case PCL6_ENC_UINT32_BOX :
	return ("uint32_box");

    case PCL6_ENC_SINT16_BOX :
	return ("sint16_box");

    case PCL6_ENC_SINT32_BOX :
	return ("sint32_box");

    case PCL6_ENC_REAL32_BOX :
	return ("real32_box");

    case PCL6_ENC_ATTR_UBYTE :
	return ("attr_ubyte");

    case PCL6_ENC_ATTR_UINT16 :
	return ("attr_uint16");

    default :
        snprintf(unknown, sizeof(unknown), "Encoding-%02X", (unsigned)encoding);
        return (unknown);
  }
}


//
// 'read_uint16()' - Read a 16-bit unsigned integer.
//

static unsigned				// O - Value or 0xffffffff on end-of-file
read_uint16(FILE *fp)			// I - File to read from
{
  unsigned char	buffer[2];		// Read buffer


  if (fread(buffer, 1, sizeof(buffer), fp) < sizeof(buffer))
    return ((unsigned)-1);
  else if (big_endian)
    return ((buffer[0] << 8) | buffer[1]);
  else
    return ((buffer[1] << 8) | buffer[0]);
}


//
// 'read_uint32()' - Read a 32-bit unsigned integer.
//

static unsigned				// O - Value or 0xffffffff on end-of-file
read_uint32(FILE *fp)			// I - File to read from
{
  unsigned char	buffer[4];		// Read buffer


  if (fread(buffer, 1, sizeof(buffer), fp) < sizeof(buffer))
    return ((unsigned)-1);
  else if (big_endian)
    return ((buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3]);
  else
    return ((buffer[3] << 24) | (buffer[2] << 16) | (buffer[1] << 8) | buffer[0]);
}
