/* config.h, configuration defines                                          */

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


/* define NON_ANSI_COMPILER for compilers without function prototyping */
/* #define NON_ANSI_COMPILER */

#ifdef NON_ANSI_COMPILER
#define _ANSI_ARGS_(x) ()
#else
#define _ANSI_ARGS_(x) x
#endif

#define RB "rb"
#define WB "wb"

#ifndef O_BINARY
#define O_BINARY 0
#endif



#define PACKAGE "mpg2ppm"
#define VERSION	"1.3"

#if defined(_M68060) || defined(mc68060)
  #define OnCPU	"68060"
#elif defined(_M68040) || defined(mc68040)
  #define OnCPU	"68040"
#elif defined(_M68030) || defined(mc68030)
  #define OnCPU	"68030"
#elif defined(_M68020) || defined(mc68020)
  #define OnCPU	"68020"
#elif defined(__MORPHOS__)
  #define OnCPU	"MorphOS"
#elif defined(WARPUP)
  #define OnCPU	"WarpOS"
#elif defined(__PPC__)
  #define OnCPU	"PPC"
#else
  #define OnCPU	""
#endif


