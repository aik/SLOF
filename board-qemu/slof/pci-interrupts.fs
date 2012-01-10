: pci-gen-irq-entry ( prop-addr prop-len config-addr -- prop-addr prop-len )
  ." SHOULD NOT GET THERE !" cr
  drop
;

: pci-set-irq-line ( config-addr -- )
  drop
;
