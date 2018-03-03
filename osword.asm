        .title ISO-9660 (CDFS) for the BBC Micro
        .module osword
        .r6500
        .area SWROM

; TODO: This probably needs to timeout
        .macro nbsywait ?lp
lp:     lda IDESTAT
        rol
        bcs lp
        .endm

        .macro ideoutw
        iny
        lda (0xF0),y
        sta IDEDATH
        dey
        lda (0xF0),y
        sta IDEDATL
        iny
        iny
        .endm

        .macro ideinw
        lda IDEDATL
        sta (0xF0),y
        iny
        lda IDEDATH
        sta (0xF0),y
        iny
        .endm

; OSWORD handler
; Entry:
;       A = 0x08, X = ROM socket number (as usual for service calls)
;       0xEF = A, 0xF0 = X, 0xF1 = Y registers passed to OSWORD
; Exit:
;       A, X, Y preserved if unclaimed
;       A = 0, X, Y not necessarily preserved if claimed
oswhndl::
        pha
        lda 0xEF        ; Check which OSWORD was called
        cmp #0xCD       ; Is it us?
        beq 1$
        pla
        rts
1$:     pla
        jmp atapipkt

; Process an ATAPI PACKET command
; Entry:
;       0xF0 = lo byte, 0xF1 = hi byte of address of command packet
;       (12 bytes), followed by the data address (another 2 bytes)
; Exit:
;       A = 0, X = error code
;       Y, 0xF0, 0xF1 not preserved
atapipkt:
        php
        sei
; Select device
        nbsywait
        lda #0x10       ; Hardcode slave for now
        sta IDEHEAD
; Issue the PACKET command
        nbsywait
        lda #0x00
        sta IDEFEAT     ; No overlap or DMA
        sta IDESCCT     ; No TCQ
        sta IDECYLL     ; Max byte count per DRQ of 0x0100 (256)
        lda #0x01
        sta IDECYLH
        lda #0xA0       ; PACKET command
        sta IDECMD
; Output command packet
        nbsywait
        lda IDESTAT     ; DRQ and not ERR?
        and #0x09
        cmp #0x08
        bne 33$
        ldy #0
0$:     ideoutw
        cpy #12
        bne 0$
; Input/output data
        lda (0xF0),y    ; Set up data address
        pha
        iny
        lda (0xF0),y
        sta 0xF1
        pla
        sta 0xF0
1$:     nbsywait
        lda IDESTAT     ; DRQ and not ERR?
        and #0x09
        cmp #0x08
33$:    bne 3$
        ldy #0
2$:     ideinw
        bne 2$
        inc 0xF1        ; Next page (size assumed 256)
        jmp 1$          ; Go back for next DRQ
; Completion
3$:     lda IDESTAT     ; ERR/CHK flag? If so, read error register
        and #0x01
        beq 4$
        lda IDEERR
4$:     tax
        lda #0
        plp
        rts

        .end
