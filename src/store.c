/* store.c, picture output routines                                         */

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
#include <fcntl.h>

#include "config.h"
#include "global.h"

/* private prototypes */

static void store _ANSI_ARGS_((char *outname, unsigned char *src[],
  int offset, int incr, int height));

static void putbyte _ANSI_ARGS_((int c));
//static void putword _ANSI_ARGS_((int w));
static void conv422to444 _ANSI_ARGS_((unsigned char *src, unsigned char *dst));
static void conv420to422 _ANSI_ARGS_((unsigned char *src, unsigned char *dst));

#define OBFRSIZE 4096
static unsigned char obfr[OBFRSIZE];
static unsigned char *optr;
static int outfile;

/*
 * store a picture as either one frame or two fields
 */
void Write_Frame(src,frame)
unsigned char *src[];
int frame;
{
  char outname[FILENAME_LENGTH];

 if(frame>=Start_Frame) {
  if(End_Frame)
   if(frame>End_Frame) exit(0);
  if (progressive_sequence || progressive_frame)
  {
    /* progressive */
    sprintf(outname,Output_Picture_Filename,OutputFrameCount++,'f');
    store(outname,src,0,Coded_Picture_Width,vertical_size);
  }
  else
  {
    /* interlaced */
    sprintf(outname,Output_Picture_Filename,frame,'a');
    store(outname,src,0,Coded_Picture_Width<<1,vertical_size>>1);

    sprintf(outname,Output_Picture_Filename,frame,'b');
    store(outname,src,
      Coded_Picture_Width,Coded_Picture_Width<<1,vertical_size>>1);
  }
 }
}

/*
 * store as PPM (PBMPLUS)
 */
static void store(outname,src,offset,incr,height)
char *outname;
unsigned char *src[];
int offset, incr, height;
{
  int i, j;
  int y, u, v, r, g, b;
  int crv, cbu, cgu, cgv;
  unsigned char *py, *pu, *pv;
//  static unsigned char tga24[14] = {0,0,2,0,0,0,0, 0,0,0,0,0,24,32};
  char header[FILENAME_LENGTH], cadena[128];
  static unsigned char *u422, *v422, *u444, *v444;

  if (chroma_format==CHROMA444)
  {
    u444 = src[1];
    v444 = src[2];
  }
  else
  {
    if (!u444)
    {
      if (chroma_format==CHROMA420)
      {
        if (!(u422 = (unsigned char *)malloc((Coded_Picture_Width>>1)
                                             *Coded_Picture_Height)))
          Error("malloc failed");
        if (!(v422 = (unsigned char *)malloc((Coded_Picture_Width>>1)
                                             *Coded_Picture_Height)))
          Error("malloc failed");
      }

      if (!(u444 = (unsigned char *)malloc(Coded_Picture_Width
                                           *Coded_Picture_Height)))
        Error("malloc failed");

      if (!(v444 = (unsigned char *)malloc(Coded_Picture_Width
                                           *Coded_Picture_Height)))
        Error("malloc failed");
    }

    if (chroma_format==CHROMA420)
    {
      conv420to422(src[1],u422);
      conv420to422(src[2],v422);
      conv422to444(u422,u444);
      conv422to444(v422,v444);
    }
    else
    {
      conv422to444(src[1],u444);
      conv422to444(src[2],v444);
    }
  }

  strcat(outname,".ppm");

  sprintf(cadena,"Saving picture %s\n",outname);
  imp(1, cadena);

  if ((outfile = open(outname,O_CREAT|O_TRUNC|O_WRONLY|O_BINARY,0666))==-1)
  {
    sprintf(Error_Text,"Couldn't create %s\n",outname);
    Error(Error_Text);
  }

  optr = obfr;

    /* PPM header */
    sprintf(header,"P6\n%d %d\n255\n",horizontal_size,height);

    for (i=0; header[i]!=0; i++)
      putbyte(header[i]);

  /* matrix coefficients */
  crv = Inverse_Table_6_9[matrix_coefficients][0];
  cbu = Inverse_Table_6_9[matrix_coefficients][1];
  cgu = Inverse_Table_6_9[matrix_coefficients][2];
  cgv = Inverse_Table_6_9[matrix_coefficients][3];
  
  for (i=0; i<height; i++)
  {
    py = src[0] + offset + incr*i;
    pu = u444 + offset + incr*i;
    pv = v444 + offset + incr*i;

    for (j=0; j<horizontal_size; j++)
    {
      u = *pu++ - 128;
      v = *pv++ - 128;
      y = 76309 * (*py++ - 16); /* (255/219)*65536 */
      r = Clip[(y + crv*v + 32768)>>16];
      g = Clip[(y - cgu*u - cgv*v + 32768)>>16];
      b = Clip[(y + cbu*u + 32786)>>16];

      putbyte(r); putbyte(g); putbyte(b);
    }
  }

  if (optr!=obfr)
    write(outfile,obfr,optr-obfr);

  close(outfile);
}

static void putbyte(c)
int c;
{
  *optr++ = c;

  if (optr == obfr+OBFRSIZE)
  {
    write(outfile,obfr,OBFRSIZE);
    optr = obfr;
  }
}


/* horizontal 1:2 interpolation filter */
static void conv422to444(src,dst)
unsigned char *src,*dst;
{
  int i, i2, w, j, im3, im2, im1, ip1, ip2, ip3;

  w = Coded_Picture_Width>>1;

  if (base.MPEG2_Flag)
  {
    for (j=0; j<Coded_Picture_Height; j++)
    {
      for (i=0; i<w; i++)
      {
        i2 = i<<1;
        im2 = (i<2) ? 0 : i-2;
        im1 = (i<1) ? 0 : i-1;
        ip1 = (i<w-1) ? i+1 : w-1;
        ip2 = (i<w-2) ? i+2 : w-1;
        ip3 = (i<w-3) ? i+3 : w-1;

        /* FIR filter coefficients (*256): 21 0 -52 0 159 256 159 0 -52 0 21 */
        /* even samples (0 0 256 0 0) */
        dst[i2] = src[i];

        /* odd samples (21 -52 159 159 -52 21) */
        dst[i2+1] = Clip[(int)(21*(src[im2]+src[ip3])
                        -52*(src[im1]+src[ip2]) 
                       +159*(src[i]+src[ip1])+128)>>8];
      }
      src+= w;
      dst+= Coded_Picture_Width;
    }
  }
  else
  {
    for (j=0; j<Coded_Picture_Height; j++)
    {
      for (i=0; i<w; i++)
      {

        i2 = i<<1;
        im3 = (i<3) ? 0 : i-3;
        im2 = (i<2) ? 0 : i-2;
        im1 = (i<1) ? 0 : i-1;
        ip1 = (i<w-1) ? i+1 : w-1;
        ip2 = (i<w-2) ? i+2 : w-1;
        ip3 = (i<w-3) ? i+3 : w-1;

        /* FIR filter coefficients (*256): 5 -21 70 228 -37 11 */
        dst[i2] =   Clip[(int)(  5*src[im3]
                         -21*src[im2]
                         +70*src[im1]
                        +228*src[i]
                         -37*src[ip1]
                         +11*src[ip2]+128)>>8];

       dst[i2+1] = Clip[(int)(  5*src[ip3]
                         -21*src[ip2]
                         +70*src[ip1]
                        +228*src[i]
                         -37*src[im1]
                         +11*src[im2]+128)>>8];
      }
      src+= w;
      dst+= Coded_Picture_Width;
    }
  }
}

/* vertical 1:2 interpolation filter */
static void conv420to422(src,dst)
unsigned char *src,*dst;
{
  int w, h, i, j, j2;
  int jm6, jm5, jm4, jm3, jm2, jm1, jp1, jp2, jp3, jp4, jp5, jp6, jp7;

  w = Coded_Picture_Width>>1;
  h = Coded_Picture_Height>>1;

  if (progressive_frame)
  {
    /* intra frame */
    for (i=0; i<w; i++)
    {
      for (j=0; j<h; j++)
      {
        j2 = j<<1;
        jm3 = (j<3) ? 0 : j-3;
        jm2 = (j<2) ? 0 : j-2;
        jm1 = (j<1) ? 0 : j-1;
        jp1 = (j<h-1) ? j+1 : h-1;
        jp2 = (j<h-2) ? j+2 : h-1;
        jp3 = (j<h-3) ? j+3 : h-1;

        /* FIR filter coefficients (*256): 5 -21 70 228 -37 11 */
        /* New FIR filter coefficients (*256): 3 -16 67 227 -32 7 */
        dst[w*j2] =     Clip[(int)(  3*src[w*jm3]
                             -16*src[w*jm2]
                             +67*src[w*jm1]
                            +227*src[w*j]
                             -32*src[w*jp1]
                             +7*src[w*jp2]+128)>>8];

        dst[w*(j2+1)] = Clip[(int)(  3*src[w*jp3]
                             -16*src[w*jp2]
                             +67*src[w*jp1]
                            +227*src[w*j]
                             -32*src[w*jm1]
                             +7*src[w*jm2]+128)>>8];
      }
      src++;
      dst++;
    }
  }
  else
  {
    /* intra field */
    for (i=0; i<w; i++)
    {
      for (j=0; j<h; j+=2)
      {
        j2 = j<<1;

        /* top field */
        jm6 = (j<6) ? 0 : j-6;
        jm4 = (j<4) ? 0 : j-4;
        jm2 = (j<2) ? 0 : j-2;
        jp2 = (j<h-2) ? j+2 : h-2;
        jp4 = (j<h-4) ? j+4 : h-2;
        jp6 = (j<h-6) ? j+6 : h-2;

        /* Polyphase FIR filter coefficients (*256): 2 -10 35 242 -18 5 */
        /* New polyphase FIR filter coefficients (*256): 1 -7 30 248 -21 5 */
        dst[w*j2] = Clip[(int)(  1*src[w*jm6]
                         -7*src[w*jm4]
                         +30*src[w*jm2]
                        +248*src[w*j]
                         -21*src[w*jp2]
                          +5*src[w*jp4]+128)>>8];

        /* Polyphase FIR filter coefficients (*256): 11 -38 192 113 -30 8 */
        /* New polyphase FIR filter coefficients (*256):7 -35 194 110 -24 4 */
        dst[w*(j2+2)] = Clip[(int)( 7*src[w*jm4]
                             -35*src[w*jm2]
                            +194*src[w*j]
                            +110*src[w*jp2]
                             -24*src[w*jp4]
                              +4*src[w*jp6]+128)>>8];

        /* bottom field */
        jm5 = (j<5) ? 1 : j-5;
        jm3 = (j<3) ? 1 : j-3;
        jm1 = (j<1) ? 1 : j-1;
        jp1 = (j<h-1) ? j+1 : h-1;
        jp3 = (j<h-3) ? j+3 : h-1;
        jp5 = (j<h-5) ? j+5 : h-1;
        jp7 = (j<h-7) ? j+7 : h-1;

        /* Polyphase FIR filter coefficients (*256): 11 -38 192 113 -30 8 */
        /* New polyphase FIR filter coefficients (*256):7 -35 194 110 -24 4 */
        dst[w*(j2+1)] = Clip[(int)( 7*src[w*jp5]
                             -35*src[w*jp3]
                            +194*src[w*jp1]
                            +110*src[w*jm1]
                             -24*src[w*jm3]
                              +4*src[w*jm5]+128)>>8];

        dst[w*(j2+3)] = Clip[(int)(  1*src[w*jp7]
                             -7*src[w*jp5]
                             +30*src[w*jp3]
                            +248*src[w*jp1]
                             -21*src[w*jm1]
                              +5*src[w*jm3]+128)>>8];
      }
      src++;
      dst++;
    }
  }
}