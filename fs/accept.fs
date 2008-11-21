\ =============================================================================
\  * Copyright (c) 2004, 2005 IBM Corporation
\  * All rights reserved. 
\  * This program and the accompanying materials 
\  * are made available under the terms of the BSD License 
\  * which accompanies this distribution, and is available at
\  * http://www.opensource.org/licenses/bsd-license.php
\  * 
\  * Contributors:
\  *     IBM Corporation - initial implementation
\ =============================================================================


\ Implementation of ACCEPT.  Nothing fancy for now; just handles CR and BS.

: TABLE-EXECUTE CREATE DOES> swap cells+ @ ?dup IF execute ELSE false THEN ;

0 VALUE accept-adr
0 VALUE accept-max
0 VALUE accept-len

: handle-backspace  accept-len ?dup IF 1- TO accept-len
   bs emit space bs emit THEN false ;

: handle-enter  space true ;

TABLE-EXECUTE handle-control
0 , 0 , 0 , 0 ,
0 , 0 , 0 , 0 ,
' handle-backspace , 0 , 0 , 0 ,
0 , ' handle-enter , 0 , 0 ,
0 , 0 , 0 , 0 ,
0 , 0 , 0 , 0 ,
0 , 0 , 0 , 0 ,
0 , 0 , 0 , 0 ,

: handle-normal
   dup emit
   accept-len accept-max < IF
   accept-adr accept-len chars+ c!
   accept-len 1+ TO accept-len
   ELSE drop THEN ;

: (accept) ( adr len -- len' )
   TO accept-max TO accept-adr 0 TO accept-len
   BEGIN key
   dup 7f = IF drop 8 THEN \ Handle DEL as if it was BS.
   dup bl < IF handle-control IF accept-len exit THEN
   ELSE handle-normal THEN
   AGAIN ;

' (accept) TO accept
