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

; OSWORD handler
; Entry:
;       A = 0x08, X = ROM socket number (as usual for service calls)
;       0xEF = A, 0xF0 = X, 0xF1 = Y registers passed to OSWORD
;       (0xEF holding OSWORD number, 0xF0 holding lo byte,
;       0xF1 holding hi byte of the address of a parameter block)
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
;       A = 0 to claim the service call
;       X, Y not preserved
;       0xEF, 0xF0, 0xF1 preserved
;       First byte of command packet overwritten by 0x00 on success,
;       contents of ATA error register on error
atapipkt:
        nbsywait
        lda #0xF0       ; Hardcode slave for now
        sta IDEHEAD
        nbsywait

        lda #0x00
        sta IDEFEAT     ; No overlap or DMA
        sta IDESCCT     ; No TCQ
        sta IDECYLL     ; Max byte count per DRQ of 0x0100 (256)
        lda #0x01
        sta IDECYLH
        lda #0xA0       ; PACKET command
        sta IDECMD

        nbsywait
        lda IDESTAT     ; DRQ?
        and #0x08
        beq 9$

        ldy #0          ; Output command packet
32$:    iny
        lda [0xF0],y
        sta IDEDATH
        dey
        lda [0xF0],y
        sta IDEDATL
        iny
        iny
        cpy #12         ; Finished command packet?
        bne 32$

        lda [0xF0],y    ; Set up data address
        sta 0x90        ; Abusing the Econet area for now
        iny
        lda [0xF0],y
        sta 0x91

4$:     nbsywait
        lda IDESTAT     ; DRQ?
        and #0x08
        beq 9$

        ldy #0
        ldx IDECYLL     ; Byte count for this DRQ
        lda IDESCCT     ; Read or write?
        ror
        ror
        bcc 61$

6$:     lda IDEDATL     ; Read as many bytes as drive specifies
        sta [0x90],y
        iny
        dex
        beq 7$
        lda IDEDATH
        sta [0x90],y
        iny
        dex
        beq 7$
        jmp 6$

61$:    iny             ; Write as many bytes as drive specifies
        lda [0x90],y
        sta IDEDATH
        dey
        lda [0x90],y
        sta IDEDATL
        iny
        dex
        beq 7$
        iny
        dex
        beq 7$
        jmp 61$

7$:     tya             ; Advance data pointer for next DRQ
        beq 71$
        clc
        adc 0x90
        sta 0x90
        bcc 4$
71$:    inc 0x91
        jmp 4$          ; And go back and check for next DRQ

9$:     ldy #0          ; Check for error
        lda IDESTAT
        and #0x01
        beq 10$
        lda IDEERR
10$:    sta [0xF0],y    ; Return error in first byte (opcode) of packet
        tya             ; Return with A=0 to claim service
        rts

        .end
