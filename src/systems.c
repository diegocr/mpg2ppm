/* systems.c, systems-specific routines                                 */

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

#include "config.h"
#include "global.h"

/* initialize buffer, call once before first getbits or showbits */

/* parse system layer, ignore everything we don't need */
void Next_Packet()
{
  unsigned int code;
  int l;

  for(;;)
  {
    code = Get_Long();

    /* remove system layer byte stuffing */
    while ((code & 0xffffff00) != 0x100)
      code = (code<<8) | Get_Byte();

    switch(code)
    {
    case PACK_START_CODE: /* pack header */
      /* skip pack header (system_clock_reference and mux_rate) */
      ld->Rdptr += 8;
      break;
    case VIDEO_ELEMENTARY_STREAM:   
      code = Get_Word();             /* packet_length */
      ld->Rdmax = ld->Rdptr + code;

      code = Get_Byte();

      if((code>>6)==0x02)
      {
        ld->Rdptr++;
        code=Get_Byte();  /* parse PES_header_data_length */
        ld->Rdptr+=code;    /* advance pointer by PES_header_data_length */
        imp(2,"MPEG-2 PES packet\n");
        return;
      }
      else if(code==0xff)
      {
        /* parse MPEG-1 packet header */
        while((code=Get_Byte())== 0xFF);
      }
       
      /* stuffing bytes */
      if(code>=0x40)
      {
        if(code>=0x80)
          imp(4,"Error in packet header\n");

        /* skip STD_buffer_scale */
        ld->Rdptr++;
        code = Get_Byte();
      }

      if(code>=0x30)
      {
        if(code>=0x40)
          imp(4,"Error in packet header\n");

        /* skip presentation and decoding time stamps */
        ld->Rdptr += 9;
      }
      else if(code>=0x20)
      {
        /* skip presentation time stamps */
        ld->Rdptr += 4;
      }
      else if(code!=0x0f)
        imp(4,"Error in packet header\n");
      return;
    case ISO_END_CODE: /* end */
      /* simulate a buffer full of sequence end codes */
      l = 0;
      while (l<2048)
      {
        ld->Rdbfr[l++] = SEQUENCE_END_CODE>>24;
        ld->Rdbfr[l++] = SEQUENCE_END_CODE>>16;
        ld->Rdbfr[l++] = SEQUENCE_END_CODE>>8;
        ld->Rdbfr[l++] = SEQUENCE_END_CODE&0xff;
      }
      ld->Rdptr = ld->Rdbfr;
      ld->Rdmax = ld->Rdbfr + 2048;
      return;
    default:
      if(code>=SYSTEM_START_CODE)
      {
        /* skip system headers and non-video packets*/
        code = Get_Word();
        ld->Rdptr += code;
      }
      else
      {
        sprintf(Error_Text,"Unexpected startcode %08x in system layer\n",code);
        imp(4, Error_Text);
      }
      break;
    }
  }
}



void Flush_Buffer32()
{
  int Incnt;

  ld->Bfr = 0;

  Incnt = ld->Incnt;
  Incnt -= 32;

  if (System_Stream_Flag && (ld->Rdptr >= ld->Rdmax-4))
  {
    while (Incnt <= 24)
    {
      if (ld->Rdptr >= ld->Rdmax)
        Next_Packet();
      ld->Bfr |= Get_Byte() << (24 - Incnt);
      Incnt += 8;
    }
  }
  else
  {
    while (Incnt <= 24)
    {
      if (ld->Rdptr >= ld->Rdbfr+2048)
        Fill_Buffer();
      ld->Bfr |= *ld->Rdptr++ << (24 - Incnt);
      Incnt += 8;
    }
  }
  ld->Incnt = Incnt;

#ifdef VERIFY 
  ld->Bitcnt += 32;
#endif /* VERIFY */
}


unsigned int Get_Bits32()
{
  unsigned int l;

  l = Show_Bits(32);
  Flush_Buffer32();

  return l;
}


int Get_Long()
{
  int i;

  i = Get_Word();
  return (i<<16) | Get_Word();
}


