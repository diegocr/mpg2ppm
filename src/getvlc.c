/* getvlc.c, variable length decoding                                       */

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

#include "config.h"
#include "global.h"
#include "getvlc.h"

/* private prototypes */
/* generic picture macroblock type processing functions */
static int Get_I_macroblock_type _ANSI_ARGS_((void));
static int Get_D_macroblock_type _ANSI_ARGS_((void));

/* spatial picture macroblock type processing functions */
static int Get_I_Spatial_macroblock_type _ANSI_ARGS_((void));
static int Get_SNR_macroblock_type _ANSI_ARGS_((void));

int Get_macroblock_type()
{
  int macroblock_type = 0;

  if (ld->scalable_mode==SC_SNR)
    macroblock_type = Get_SNR_macroblock_type();
  else
  {
    switch (picture_coding_type)
    {
    case I_TYPE:
      macroblock_type = ld->pict_scal ? Get_I_Spatial_macroblock_type() : Get_I_macroblock_type();
      break;
    case D_TYPE:
      macroblock_type = Get_D_macroblock_type();
      break;
    default:
      break;
    }
  }

  return macroblock_type;
}

static int Get_I_macroblock_type()
{
  if (Get_Bits1())
  {
    return 1;
  }

  if (!Get_Bits1())
  {
//    imp(2,"Invalid macroblock_type code\n");
//    Fault_Flag = 1;
   if(Do_Debug) imp(5,"end of file reached ? ...\n");
    else exit(0); /* end of file reached ? ... */
  }

  return 17;
}

static int Get_D_macroblock_type()
{
  if (!Get_Bits1())
  {
    imp(2,"Invalid macroblock_type code\n");
    Fault_Flag=1;
  }

  return 1;
}

/* macroblock_type for pictures with spatial scalability */
static int Get_I_Spatial_macroblock_type()
{
  int code;

  code = Show_Bits(4);

  if (code==0)
  {
    imp(2,"Invalid macroblock_type code\n");
    Fault_Flag = 1;
    return 0;
  }

  Flush_Buffer(spIMBtab[code].len);
  return spIMBtab[code].val;
}

static int Get_SNR_macroblock_type()
{
  int code;

  code = Show_Bits(3);

  if (code==0)
  {
    imp(2,"Invalid macroblock_type code\n");
    Fault_Flag = 1;
    return 0;
  }

  Flush_Buffer(SNRMBtab[code].len);

  return SNRMBtab[code].val;
}

int Get_motion_code()
{
  int code;

  if (Get_Bits1())
  {
    return 0;
  }

  if ((code = Show_Bits(9))>=64)
  {
    code >>= 6;
    Flush_Buffer(MVtab0[code].len);

    return Get_Bits1()?-MVtab0[code].val:MVtab0[code].val;
  }

  if (code>=24)
  {
    code >>= 3;
    Flush_Buffer(MVtab1[code].len);

    return Get_Bits1()?-MVtab1[code].val:MVtab1[code].val;
  }

  if ((code-=12)<0)
  {
/* HACK */
    sprintf(Error_Text,"Invalid motion_vector code (MBA %d, pic %d)\n", global_MBA, global_pic);
    imp(2, Error_Text);
    Fault_Flag=1;
    return 0;
  }

  Flush_Buffer(MVtab2[code].len);

  return Get_Bits1() ? -MVtab2[code].val : MVtab2[code].val;
}

/* get differential motion vector (for dual prime prediction) */
int Get_dmvector()
{
  if (Get_Bits(1))
  {
    return Get_Bits(1) ? -1 : 1;
  }
  else
  {
    return 0;
  }
}

int Get_coded_block_pattern()
{
  int code;

  if ((code = Show_Bits(9))>=128)
  {
    code >>= 4;
    Flush_Buffer(CBPtab0[code].len);

    return CBPtab0[code].val;
  }

  if (code>=8)
  {
    code >>= 1;
    Flush_Buffer(CBPtab1[code].len);

    return CBPtab1[code].val;
  }

  if (code<1)
  {
    imp(2,"Invalid coded_block_pattern code\n");
    Fault_Flag = 1;
    return 0;
  }

  Flush_Buffer(CBPtab2[code].len);

  return CBPtab2[code].val;
}

int Get_macroblock_address_increment()
{
  int code, val;

  val = 0;

  while ((code = Show_Bits(11))<24)
  {
    if (code!=15) /* if not macroblock_stuffing */
    {
      if (code==8) /* if macroblock_escape */
      {
        val+= 33;
      }
//      else
//      {
//        imp(2,"Invalid macroblock_address_increment code\n");
//        Fault_Flag = 1;
//        return 1;
//      }
    }
    else /* macroblock suffing */
    {
	/* HERE WAS A 'TRACE' CONDITION */
    }

    Flush_Buffer(11);
  }

  /* macroblock_address_increment == 1 */
  /* ('1' is in the MSB position of the lookahead) */
  if (code>=1024)
  {
    Flush_Buffer(1);
    return val + 1;
  }

  /* codes 00010 ... 011xx */
  if (code>=128)
  {
    /* remove leading zeros */
    code >>= 6;
    Flush_Buffer(MBAtab1[code].len);

    return val + MBAtab1[code].val;
  }
  
  /* codes 00000011000 ... 0000111xxxx */
  code-= 24; /* remove common base */
  Flush_Buffer(MBAtab2[code].len);

  return val + MBAtab2[code].val;
}

/* combined MPEG-1 and MPEG-2 stage. parse VLC and 
   perform dct_diff arithmetic.

   MPEG-1:  ISO/IEC 11172-2 section
   MPEG-2:  ISO/IEC 13818-2 section 7.2.1 
   
   Note: the arithmetic here is presented more elegantly than
   the spec, yet the results, dct_diff, are the same.
*/

int Get_Luma_DC_dct_diff()
{
  int code, size, dct_diff;

  /* decode length */
  code = Show_Bits(5);

  if (code<31)
  {
    size = DClumtab0[code].val;
    Flush_Buffer(DClumtab0[code].len);
  }
  else
  {
    code = Show_Bits(9) - 0x1f0;
    size = DClumtab1[code].val;
    Flush_Buffer(DClumtab1[code].len);
  }

  if (size==0)
    dct_diff = 0;
  else
  {
    dct_diff = Get_Bits(size);

    if ((dct_diff & (1<<(size-1)))==0)
      dct_diff-= (1<<size) - 1;
  }

  return dct_diff;
}


int Get_Chroma_DC_dct_diff()
{
  int code, size, dct_diff;

  /* decode length */
  code = Show_Bits(5);

  if (code<31)
  {
    size = DCchromtab0[code].val;
    Flush_Buffer(DCchromtab0[code].len);
  }
  else
  {
    code = Show_Bits(10) - 0x3e0;
    size = DCchromtab1[code].val;
    Flush_Buffer(DCchromtab1[code].len);
  }

  if (size==0)
    dct_diff = 0;
  else
  {
    dct_diff = Get_Bits(size);

    if ((dct_diff & (1<<(size-1)))==0)
      dct_diff-= (1<<size) - 1;
  }

  return dct_diff;
}
