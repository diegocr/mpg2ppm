/* verify.c		Bitstream verification routines */

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


#ifdef VERIFY 

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <math.h>     /* needed for ceil() */

#include "config.h"
#include "global.h"


/* 
   Check picture headers:  due to the VBV definition of picture data,
   this routine must be called immediately before any picture data 
   is parsed. (before the first slice start code, including any slice 
   start code stuffing).
*/


static void Check_VBV_Delay _ANSI_ARGS_((int Bitstream_Framenum, int Sequence_Framenum));


void Check_Headers(Bitstream_Framenum, Sequence_Framenum)
int Bitstream_Framenum;
int Sequence_Framenum;
{


  if((!low_delay)&&(vbv_delay!=0)&&(vbv_delay!=0xFFFF))
    Check_VBV_Delay(Bitstream_Framenum, Sequence_Framenum);

  /* clear out the header tracking variables so we have an accurate 
     count next time */
  Clear_Verify_Headers();
}



/* 
 * Verify vbv_delay value in picture header 
 * (low_delay==1 checks not implemented. this does not exhaustively test all 
 *  possibilities suggested in ISO/IEC 13818-2 Annex C.  It only checks
 *  for constant rate streams)
 *
 * Q:how do we tell a variable rate stream from a constant rate stream anyway?
 *   it's not as simple as vbv_delay==0xFFFF, since we need meaningful 
 *   vbv_delay values to calculate the piecewise rate in the first place!
 *
 * Also: no special provisions at the beginning or end of a sequence
 */

static void Check_VBV_Delay(Bitstream_Framenum, Sequence_Framenum)
int Bitstream_Framenum;
int Sequence_Framenum;
{
  double B;   /* buffer size                   */
  double Bn;  /* buffer fullness for picture n */
  double R;   /* bitrate                       */
  double I;   /* time interval (t[n+1] - t[n]) */
  double T;   /* inverse of the frame rate (frame period) */

  int d;
  int internal_vbv_delay;
  
  static int previous_IorP_picture_structure;
  static int previous_IorP_repeat_first_field;
  static int previous_IorP_top_field_first;
  static int previous_vbv_delay;
  static int previous_bitstream_position;

  static double previous_Bn;
  static double E;      /* maximum quantization error or mismatch */

  

  if((Sequence_Framenum==0)&&(!Second_Field)) 
  {  /* first coded picture of sequence */

    R = bit_rate;

    /* the initial buffer occupancy is taken on faith
       that is, we believe what is transmitted in the first coded picture header
       to be the true/actual buffer occupancy */
    
    Bn = (R * (double) vbv_delay) / 90000.0;
    B = 16 * 1024 * vbv_buffer_size;

    
    /* maximum quantization error in bitrate (bit_rate_value is quantized/
       rounded-up to units of 400 bits/sec as per ISO/IEC 13818-2 
       section 6.3.3 */
    
    E = (400.0/frame_rate) + 400;

  }
  else /* not the first coded picture of sequence */
  {

    /* derive the interval (I).  The interval tells us how many constant rate bits
     * will have been downloaded to the buffer during the current picture period
     *
     * interval assumes that: 
     *  1. whilst we are decoding the current I or P picture, we are displaying 
     *     the previous I or P picture which was stored in the reorder
     *     buffer (pointed to by forward_reference_frame in this implementation)
     *
     *  2. B pictures are output ("displayed") at the time when they are decoded 
     * 
     */

    if(progressive_sequence) /* Annex C.9 (progressive_sequence==1, low_delay==0) */
    {

      T = 1/frame_rate; /* inverse of the frame rate (frame period) */

      if(picture_coding_type==B_TYPE)
      {
        if(repeat_first_field==1)
        {
          if(top_field_first==1)
            I = T*3;  /* three frame periods */
          else
            I = T*2;  /* two frame periods */
        }
        else
          I = T;      /* one frame period */
      }
      else /* P or I frame */
      {
        if(previous_IorP_repeat_first_field==1)
        {
          if(previous_IorP_top_field_first==1)
            I = 3*T;
          else
            I = 2*T;
        }
        else
          I = T;
      }
    }
    else /* Annex C.11 (progressive_sequence==0, low_delay==0) */
    {
      
      T = 1/(2*frame_rate); /* inverse of two times the frame rate (field period) */

      if(picture_coding_type==B_TYPE)
      {
        if(picture_structure==FRAME_PICTURE)
        {
          if(repeat_first_field==0)
            I = 2*T;  /* two field periods */
          else
            I = 3*T;  /* three field periods */
        }
        else /* B field */
        {
          I = T;      /* one field period */
        }
      }
      else /* I or P picture */
      {
        if(picture_structure==FRAME_PICTURE)
        {
          if(previous_IorP_repeat_first_field==0)
            I = 2*T;
          else
            I = 3*T;
        }
        else
        {
          if(Second_Field==0)  /* first field of current frame */
            I = T;
          else /* second field of current frame */
          {
            /* formula: previous I or P display period (2*T or 3*T) minus the 
               very recent decode period (T) of the first field of the current 
               frame */

            if(previous_IorP_picture_structure!=FRAME_PICTURE 
              || previous_IorP_repeat_first_field==0)
              I = 2*T - T;  /* a net of one field period */ 
            else if(previous_IorP_picture_structure==FRAME_PICTURE 
              && previous_IorP_repeat_first_field==1)
              I = 3*T - T;  /* a net of two field periods */
          }
        }
      }
    }

    /* derive coded size of previous picture */
    d  = ld->Bitcnt - previous_bitstream_position;

    /* Rate = Distance/Time */

    /* piecewise constant rate (variable rate stream) calculation
     * R =  ((double) d /((previous_vbv_delay - vbv_delay)/90000 + I));
     */

    R = bit_rate;

    /* compute buffer fullness just before removing picture n 
     *
     * Bn = previous_Bn + (I*R) - d;     (recursive formula)
     * 
     *   where:
     *
     *    n           is the current picture
     *
     *    Bn          is the buffer fullness for the current picture
     *
     *    previous_Bn is the buffer fullness of the previous picture
     *
     *    (I*R )      is the bits accumulated during the current picture 
     *                period
     *
     *    d           is the number of bits removed during the decoding of the 
     *                previous picture
     */

    Bn = previous_Bn + (I*R) - d;

    /* compute internally derived vbv_delay (rouding up with ceil()) */
    internal_vbv_delay = (int) ceil((90000 * Bn / bit_rate));

  } /* not the first coded picture of sequence */


  /* update generic tracking variables */
  previous_bitstream_position = ld->Bitcnt ;
  previous_vbv_delay          = vbv_delay;
  previous_Bn                 = Bn;

  /* reference picture: reordered/delayed output picture */
  if(picture_coding_type!=B_TYPE)
  {
    previous_IorP_repeat_first_field = repeat_first_field;
    previous_IorP_top_field_first    = top_field_first;
    previous_IorP_picture_structure  = picture_structure;
  }

}



/* variables to keep track of the occurance of redundant headers between pictures */
void Clear_Verify_Headers()
{
  verify_sequence_header = 0;
  verify_group_of_pictures_header = 0;
  verify_picture_header = 0;
  verify_slice_header = 0;
  verify_sequence_extension = 0;
  verify_sequence_display_extension = 0;
  verify_quant_matrix_extension = 0;
  verify_sequence_scalable_extension = 0;
  verify_picture_display_extension = 0;
  verify_picture_coding_extension = 0;
  verify_picture_spatial_scalable_extension = 0;
  verify_picture_temporal_scalable_extension = 0;
  verify_copyright_extension = 0;
}

#endif /* VERIFY */

