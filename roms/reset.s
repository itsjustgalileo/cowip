    .org $8000

reset:
    ldx #$ff
    txs
    cli

main:

.loop:
    jmp .loop
    
nmi:
irq:
    rti
    
    .org $FFFA
    .word nmi
    .word reset
    .word irq
    
