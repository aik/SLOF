new-device

VALUE sudev

s" slofdev.fs" included
sudev slof-dev>port l@ dup set-unit encode-phys " reg" property
sudev slof-dev>udev @ VALUE udev

s" usb-keyboard" device-name
s" keyboard" device-type
s" EN" encode-string s" language" property
s" keyboard" get-node node>path set-alias

: open   ( -- true | false )
    TRUE
;

\ method to check if a key is present in output buffer
\ used by 'term-io.fs'
: key-available? ( -- true|false )
    false
;

: read                     ( addr len -- actual )
    2drop 0
;

."     USB Keyboard " cr
finish-device
