new-device

VALUE usbdev

s" slofdev.fs" included
usbdev slof-dev>port l@ dup set-unit encode-phys " reg" property
s" disk" device-name
s" block" device-type

\ s" disk" get-node node>path set-alias

: open   ( -- true | false )
    ." Opening disk device " cr
    \ usbdev slof-dev>udev @ OPEN-KEYB-DEVICE
;

: close  ( -- )
    ." Closing disk device " cr
;

: read                     ( addr len -- actual )
    nip ." read called " cr
;

: read-blocks  ( address block# #blocks -- #read-blocks )
    nip nip ." read-blocks called " cr
;

."     USB Storage " cr

finish-device
