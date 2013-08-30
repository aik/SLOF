4096 VALUE size
: ibm,client-architecture-support         ( vec -- err? )
    \ Store require parameters in nvram
    \ to come back to right boot device

    \ Allocate memory for H_CALL
    size alloc-mem                        ( vec memaddr )
    swap over
    \ FIXME: convert memaddr to phys
    size                                  ( memaddr vec memaddr size )
    \ make h_call to hypervisor
    hv-cas 0= IF
	." hv-cas succeeded " cr
	\ Make required changes
	FALSE
    ELSE
	." hv-cas failed  " TRUE
    THEN
    >r size free-mem r>
;
