/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *  Copyright 2005, 2006 by
 *  Florian Schulze, Colin Phipps, Neil Stevens, Andrey Budko
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 *  02111-1307, USA.
 *
 * DESCRIPTION:
 *  MapObj data. Map Objects or mobjs are actors, entities,
 *  thinker, take-your-pick... anything that moves, acts, or
 *  suffers state changes of more or less violent nature.
 *
 *-----------------------------------------------------------------------------*/

#ifndef __D_THINK__
#define __D_THINK__

#ifdef __GNUG__
#pragma interface
#endif

/* Forward declarations of types used in some prototypes */
typedef struct mobj_s mobj_t;
typedef struct player_s player_t;
typedef struct pspdef_s pspdef_t;
typedef struct scroll_s scroll_t;
typedef struct thinker_s thinker_t;
typedef struct ceiling_s ceiling_t;
typedef struct vldoor_s vldoor_t;
typedef struct plat_s plat_t;
typedef struct floormove_s floormove_t;
typedef struct elevator_s elevator_t;
typedef struct fireflicker_s fireflicker_t;
typedef struct glow_s glow_t;
typedef struct lightflash_s lightflash_t;
typedef struct strobe_s strobe_t;

/*
 * Experimental stuff.
 * To compile this as "ANSI C with classes"
 *  we will need to handle the various
 *  action functions cleanly.
 */
// killough 11/98: convert back to C instead of C++
//typedef  void (*actionf_t)();
typedef  void (*actionf_v)();
typedef  void (*actionf_p1)( player_t* );
typedef  void (*actionf_p2)( player_t*, pspdef_t* );
typedef void (*actionf_m1)( mobj_t* );
typedef void (*actionf_s1)( scroll_t* );
typedef void (*actionf_t1)( thinker_t*);
typedef void (*actionf_c1)( ceiling_t* );
typedef void (*actionf_d1)( vldoor_t* );
typedef void (*actionf_l1)( plat_t* );
typedef void (*actionf_f1)( floormove_t* );
typedef void (*actionf_e1)( elevator_t* );
typedef void (*actionf_r1)( fireflicker_t* );
typedef void (*actionf_g1)( glow_t* );
typedef void (*actionf_i1)( lightflash_t* );
typedef void (*actionf_o1)( strobe_t* );

/* Note: In d_deh.c you will find references to these
 * wherever code pointers and function handlers exist
 */
typedef union
{
  actionf_v     acv;  /// No parameters
  actionf_p1    acp1; /// player_t *
  actionf_p2    acp2; /// player_t *, pspdef_t *
  actionf_m1    acm1; /// mobj_t *
  actionf_s1    acs1; /// scroll_t *
  actionf_t1    act1; /// thinker_t*
  actionf_c1    acc1; /// ceiling_t *
  actionf_d1    acd1; /// vldoor_t *
  actionf_l1    acl1; /// plat_t *
  actionf_f1    acf1; /// floormove_t *
  actionf_e1    ace1; /// elevator_t *
  actionf_r1    acr1; /// fireflicker_t *
  actionf_g1    acg1; /// glow_t *
  actionf_i1    aci1; /// lightflash_t *
  actionf_o1    aco1; /// strobe_t *

} actionf_t;

/* Historically, "think_t" is yet another
 *  function pointer to a routine to handle
 *  an actor.
 */
typedef actionf_t  think_t;


/* Doubly linked list of actors. */
typedef struct thinker_s
{
  struct thinker_s*   prev;
  struct thinker_s*   next;
  think_t             function;

} thinker_t;

#endif
