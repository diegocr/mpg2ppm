/* mpg2ppm.c, main(), initialization, option processing                    */

/* Copyright (C) 1996, MPEG Software Simulation Group. All Rights Reserved. */

/*
 * Disclaimer of Warranty
 *
 * These software programs are available to the user without any license fee or
 * royalty on an "as is" basis.  The MPEG Software Simulation Group disclaims
 * any and all warranties, whether express, implied, or statuary, including any
 * implied warranties or merchantability or of fitness for a particular
 * purpose.  In no event shall the copyright-holder be liable for any
 * incidental, punitive, or consequential damages of any kind whatsoever
 * arising from the use of these programs.
 *
 * This disclaimer of warranty extends to the user of these programs and user's
 * customers, employees, agents, transferees, successors, and assigns.
 *
 * The MPEG Software Simulation Group does not represent or warrant that the
 * programs furnished hereunder are free of infringement of any third-party
 * patents.
 *
 * Commercial implementations of MPEG-1 and MPEG-2 video, including shareware,
 * are subject to royalty fees to patent holders.  Many of these patents are
 * general enough such that they are unavoidable regardless of implementation
 * design.
 *
 */
/* Modifications and enhancements  (C) 2003/2004 Diego Casorran  */

/* These modifications are free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <time.h>

#define GLOBAL
#include "config.h"
#include "global.h"

/* private prototypes */
static int  video_sequence _ANSI_ARGS_((int *framenum));
static int Decode_Bitstream _ANSI_ARGS_((void));
static int  Headers _ANSI_ARGS_((void));
static void Initialize_Sequence _ANSI_ARGS_((void));
static void Initialize_Decoder _ANSI_ARGS_((void));
static void Deinitialize_Sequence _ANSI_ARGS_((void));
static void Process_Options _ANSI_ARGS_((int argc, char *argv[]));
char *getime();

#ifdef __amigaos__
# define oput stdout
#else
# define oput stderr
#endif

#ifndef NOVID
   static const char verz[] = 
	"$VER: " PACKAGE " v"VERSION " ["OnCPU"] (c)2003 Diego Casorran";
   char auth[] = 
	"$AUTH: " PACKAGE " is based on mpeg2decode, (C)1996 MPEG SSGroup";
#endif


int main(argc,argv)
int argc;
char *argv[];
{
  int ret, code;

  /* decode command line arguments */
  Process_Options(argc,argv);

  ld = &base; /* select base layer context */

  /* open MPEG base layer bitstream file(s) */
  /* NOTE: this is either a base layer stream or a spatial enhancement stream */
  if ((base.Infile=open(Main_Bitstream_Filename,O_RDONLY|O_BINARY))<0)
  {
    sprintf(Error_Text,"Base layer input file %s not found\n", Main_Bitstream_Filename);
    imp(4, Error_Text);
  }
  
  if(base.Infile != 0)
  {
    Initialize_Buffer(); 
  
    if(Show_Bits(8)==0x47)
    {
      sprintf(Error_Text,"Decoder currently does not parse transport streams\n");
      Error(Error_Text);
    }

    imp(1, "Proccessing a");
    next_start_code();
    code = Show_Bits(32);

    switch(code)
    {
    case SEQUENCE_HEADER_CODE:
      break;
      if(!DO_Quiet) fprintf(oput, " Video ");
    case PACK_START_CODE:
      System_Stream_Flag = 1;
      if(!DO_Quiet) fprintf(oput, " Multiplexed ");
    case VIDEO_ELEMENTARY_STREAM:
      System_Stream_Flag = 1;
      break;
    default:
      if(!DO_Quiet) fprintf(oput, " Unknow Stream.\n");
      sprintf(Error_Text,"Unable to recognize %s\n", Main_Bitstream_Filename);
      Error(Error_Text);
      break;
    }
    if(!DO_Quiet) fprintf(oput, "Stream.\n");
    imp(1,"Jumping to ");
    if(!DO_Quiet) fprintf(oput, "%s #%d, please wait...\n", emode ? "GOP" : "FRAME",  Start_Frame);

    lseek(base.Infile, 0l, 0);
    Initialize_Buffer(); 
  }

  if(base.Infile!=0)
  {
    lseek(base.Infile, 0l, 0);
  }

  Initialize_Buffer(); 

  Initialize_Decoder();

  ret = Decode_Bitstream();

  close(base.Infile);

  return 0;
}

/* IMPLEMENTAION specific rouintes */
static void Initialize_Decoder()
{
  int i;

  /* Clip table */
  if (!(Clip=(unsigned char *)malloc(1024)))
    Error("Clip[] malloc failed\n");

  Clip += 384;

  for (i=-384; i<640; i++)
    Clip[i] = (i<0) ? 0 : ((i>255) ? 255 : i);

  /* IDCT */
    Initialize_Fast_IDCT();
}


/* mostly IMPLEMENTAION specific rouintes */
static void Initialize_Sequence()
{
  int cc, size;
  static int Table_6_20[3] = {6,8,12};

  /* check scalability mode of enhancement layer */


  /* force MPEG-1 parameters for proper decoder behavior */
  /* see ISO/IEC 13818-2 section D.9.14 */
  if (!base.MPEG2_Flag)
  {
    progressive_sequence = 1;
    progressive_frame = 1;
    picture_structure = FRAME_PICTURE;
    frame_pred_frame_dct = 1;
    chroma_format = CHROMA420;
    matrix_coefficients = 5;
  }

  /* round to nearest multiple of coded macroblocks */
  /* ISO/IEC 13818-2 section 6.3.3 sequence_header() */
  mb_width = (horizontal_size+15)/16;
  mb_height = (base.MPEG2_Flag && !progressive_sequence) ? 2*((vertical_size+31)/32)
                                        : (vertical_size+15)/16;

  Coded_Picture_Width = 16*mb_width;
  Coded_Picture_Height = 16*mb_height;

  /* ISO/IEC 13818-2 sections 6.1.1.8, 6.1.1.9, and 6.1.1.10 */
  Chroma_Width = (chroma_format==CHROMA444) ? Coded_Picture_Width
                                           : Coded_Picture_Width>>1;
  Chroma_Height = (chroma_format!=CHROMA420) ? Coded_Picture_Height
                                            : Coded_Picture_Height>>1;
  
  /* derived based on Table 6-20 in ISO/IEC 13818-2 section 6.3.17 */
  block_count = Table_6_20[chroma_format-1];

  for (cc=0; cc<3; cc++)
  {
    if (cc==0)
      size = Coded_Picture_Width*Coded_Picture_Height;
    else
      size = Chroma_Width*Chroma_Height;

    if (!(backward_reference_frame[cc] = (unsigned char *)malloc(size)))
      Error("backward_reference_frame[] malloc failed\n");

    if (!(forward_reference_frame[cc] = (unsigned char *)malloc(size)))
      Error("forward_reference_frame[] malloc failed\n");

    if (!(auxframe[cc] = (unsigned char *)malloc(size)))
      Error("auxframe[] malloc failed\n");



    if (base.scalable_mode==SC_SPAT)
    {
      /* this assumes lower layer is 4:2:0 */
      if (!(llframe0[cc] = (unsigned char *)malloc((lower_layer_prediction_horizontal_size*lower_layer_prediction_vertical_size)/(cc?4:1))))
        Error("llframe0 malloc failed\n");
      if (!(llframe1[cc] = (unsigned char *)malloc((lower_layer_prediction_horizontal_size*lower_layer_prediction_vertical_size)/(cc?4:1))))
        Error("llframe1 malloc failed\n");
    }
  }

  /* SCALABILITY: Spatial */
  if (base.scalable_mode==SC_SPAT)
  {
    if (!(lltmp = (short *)malloc(lower_layer_prediction_horizontal_size*((lower_layer_prediction_vertical_size*vertical_subsampling_factor_n)/vertical_subsampling_factor_m)*sizeof(short))))
      Error("lltmp malloc failed\n");
  }
}

void imp(ec,text)
int ec;
char *text;
{
 switch(ec) {
  case 1:
    if(!DO_Quiet) fprintf(oput, "   INFO: [%s] %s", getime(), text);
    break;
  case 2:
    if(!DO_Quiet) fprintf(oput, "++ WARN: [%s] %s", getime(), text);
    break;
  case 3:
    if(!DO_Quiet) fprintf(oput, "**ERROR: [%s] %s", getime(), text);
    break;
  case 4:
    fprintf(oput, "\n>:FATAL: [%s] %s\n", getime(), text);
    exit(1);
    break;
  case 5:
    if(!DO_Quiet) fprintf(oput, "  DEBUG: [%s] %s", getime(), text);
    break;
  default:
    break;
 }
}

char *getime() {
time_t t = time(NULL);
struct tm *ft = localtime(&t);
char *hora = asctime(ft) + 11;
hora[(strlen(hora)-6)] = '\0';
return((char *)hora);
}

/* option processing */
static void Process_Options(argc,argv)
int argc;                  /* argument count  */
char *argv[];              /* argument vector */
{
int o, c; char *Out_Mode;
 Do_Debug=0; Loop_Frame=0; fnum=0; DO_Quiet=0; GOP_Count=0; OutputFrameCount=0;

#ifdef __amigaos__
#define TEMPLATE "INPUT/A  OUTPUT/A  START/A/N  END/A/N  MODE/A  FOFN/N  QUIET/S  DEBUG/S"
#else
#define TEMPLATE "INPUT OUTPUT START END MODE FOFN QUIET DEBUG"
#endif

//#define nfo(x) imp(1,x)
#define nfo(x) fprintf(oput, x)

  if (argc<6)
  {
    nfo("\n");
//    nfo("\n");
    fprintf(oput, "%s\n", verz+5);
    nfo("\n");
    nfo("Usage: mpg2ppm " TEMPLATE "\n");
    nfo("\n");
    nfo("\n");
    nfo("Options:\n");
    nfo("\n");
    nfo("     INPUT      A MPEG Video-Stream\n");
    nfo("    OUTPUT      Output pattern picture filename\n");
    nfo("     START      Extract from this frame/gop\n");
    nfo("       END      Stop at this frame/gop\n");
    nfo("      MODE      MUST be GOP or FRAME, depending what you want to extract.\n");
    nfo("      FOFN      First Output Frame Number.\n");
    nfo("     QUIET      do NOT print to stdout.\n");
    nfo("                (if it is set, DEBUG take no effect)\n");
    nfo("     DEBUG      print some info about the internal process.\n");
    nfo("\n");
    nfo("Example: mpg2ppm film.mpg pic%%d 69 0 GOP (END = 0 mean to the end)\n");
    nfo("\n");
   exit(0);
  }

  Main_Bitstream_Filename = argv[1];
  Output_Picture_Filename = argv[2];
  Start_Frame = atoi(argv[3]);
  End_Frame = atoi(argv[4]);
  Out_Mode = argv[5];

  if(!strcmp(Out_Mode, "GOP")) emode = 1;
  else if(!strcmp(Out_Mode, "FRAME")) emode = 0;
  else imp(4,"Mode MUST be 'GOP' or 'FRAME' !\n");
  
  if(isdigit(argv[6][0])) {
  	 OutputFrameCount = atoi(argv[6]);
  	 c = 7;
  } else c = 6;

  if(argv[c])
   for(o=c; o<(c+2); o++) {
    if(!strcmp(argv[o], "QUIET")) DO_Quiet++;
    else if(!strcmp(argv[o], "DEBUG")) Do_Debug++;
    else if(argv[o]) {
	sprintf(Error_Text, "Bad option entered! (%s)\n", argv[o]);
	Error(Error_Text);
    }
   }

  if(End_Frame)
   if(Start_Frame>End_Frame)
    End_Frame = Start_Frame;
}


static int Headers()
{
  int ret;

  ld = &base;
  

  /* return when end of sequence (0) or picture
     header has been parsed (1) */

  ret = Get_Hdr();
  return ret;
}



static int Decode_Bitstream()
{
  int ret;
  int Bitstream_Framenum;

  Bitstream_Framenum = 0;

  for(;;)
  {

#ifdef VERIFY
    Clear_Verify_Headers();
#endif /* VERIFY */

    ret = Headers();
    
    if(ret==1)
    {
      ret = video_sequence(&Bitstream_Framenum);
    }
    else
      return(ret);
  }

}


static void Deinitialize_Sequence()
{
  int i;

  /* clear flags */
  base.MPEG2_Flag=0;

  for(i=0;i<3;i++)
  {
    free(backward_reference_frame[i]);
    free(forward_reference_frame[i]);
    free(auxframe[i]);

    if (base.scalable_mode==SC_SPAT)
    {
     free(llframe0[i]);
     free(llframe1[i]);
    }
  }

  if (base.scalable_mode==SC_SPAT)
    free(lltmp);

}


static int video_sequence(Bitstream_Framenumber)
int *Bitstream_Framenumber;
{
  int Bitstream_Framenum;
  int Sequence_Framenum;
  int Return_Value;

  Bitstream_Framenum = *Bitstream_Framenumber;
  Sequence_Framenum=0;

  Initialize_Sequence();

  /* decode picture whose header has already been parsed in 
     Decode_Bitstream() */


  Decode_Picture(Bitstream_Framenum, Sequence_Framenum);

  /* update picture numbers */
  if (!Second_Field)
  {
    Bitstream_Framenum++;
    Sequence_Framenum++;
  }

  /* loop through the rest of the pictures in the sequence */
  while ((Return_Value=Headers()))
  {
    Decode_Picture(Bitstream_Framenum, Sequence_Framenum);

    if (!Second_Field)
    {
      Bitstream_Framenum++;
      Sequence_Framenum++;
    }
  }

  /* put last frame */
  if (Sequence_Framenum!=0)
  {
    Output_Last_Frame_of_Sequence(Bitstream_Framenum);
  }

  Deinitialize_Sequence();

#ifdef VERIFY
    Clear_Verify_Headers();
#endif /* VERIFY */

  *Bitstream_Framenumber = Bitstream_Framenum;
  return(Return_Value);
}

