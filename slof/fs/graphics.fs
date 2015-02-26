\ *****************************************************************************
\ * Copyright (c) 2015 IBM Corporation
\ * All rights reserved.
\ * This program and the accompanying materials
\ * are made available under the terms of the BSD License
\ * which accompanies this distribution, and is available at
\ * http://www.opensource.org/licenses/bsd-license.php
\ *
\ * Contributors:
\ *     IBM Corporation - initial implementation
\ ****************************************************************************/

\ Provide some of the functions that are defined in the
\ "OF Recommended Practice: 8bit Graphics Extension" document

: draw-rectangle ( adr x y w h -- )
   frame-buffer-adr 0= IF 4drop drop EXIT THEN
   0 ?DO
      4dup drop                              ( adr x y w adr x y )
      \ calculate offset into framebuffer: ((y + i) * width + x) * depth
      i + screen-width * + screen-depth *    ( adr x y w adr offs )
      frame-buffer-adr +                     ( adr x y w adr fb_adr )
      over 3 pick screen-depth * i * +       ( adr x y w adr fb_adr src )
      swap 3 pick screen-depth *             ( adr x y w adr src fb_adr len )
      rmove                                  \ copy line ( adr x y w adr )
      drop                                   ( adr x y w )
   LOOP
   4drop
;

: fill-rectangle ( col x y w h -- )
   frame-buffer-adr 0= IF 4drop drop EXIT THEN
   0 ?DO
      4dup drop                              ( col x y w col x y )
      \ calculate offset into framebuffer: ((y + i) * width + x) * depth
      i + screen-width * + screen-depth *    ( col x y w col offs )
      frame-buffer-adr +                     ( col x y w col adr )
      2 pick screen-depth * 2 pick           ( col x y w col adr len col )
      rfill                                  \ draw line ( col x y w col )
      drop                                   ( col x y w )
   LOOP
   4drop
;

: read-rectangle ( adr x y w h -- )
   frame-buffer-adr 0= IF 4drop drop EXIT THEN
   0 ?DO
      4dup drop                              ( adr x y w adr x y )
      \ calculate offset into framebuffer: ((y + i) * width + x) * depth
      i + screen-width * + screen-depth *    ( adr x y w adr offs )
      frame-buffer-adr +                     ( adr x y w adr fb_adr )
      over 3 pick screen-depth * i * +       ( adr x y w adr fb_adr dst )
      3 pick                                 ( adr x y w adr fb_adr dst w )
      rmove                                  \ copy line ( adr x y w adr )
      drop                                   ( adr x y w )
   LOOP
   4drop
;

: dimensions ( -- width height )
   screen-width screen-height
;
