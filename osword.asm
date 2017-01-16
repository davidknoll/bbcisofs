        .title ISO-9660 (CDFS) for the BBC Micro
        .module osword
        .r6500
        .area SWROM

; To make it easier to address the device differently
        .macro iolda addr
        lda addr
        .endm

        .macro iosta addr
        sta addr
        .endm

; TODO: This probably needs to timeout
        .macro nbsywait ?lp
lp:     iolda IDESTAT
        rol
        bcs lp
        .endm

; OSWORD handler
; Entry:
;       A = 0x08, X = ROM socket number (as usual for service calls)
;       0xEF = A, 0xF0 = X, 0xF1 = Y registers passed to OSWORD
; Exit:
;       A, X, Y preserved if unclaimed
;       A = 0, X, Y not preserved if claimed
;       0xF0, 0xF1 preserved
;       Data at (0xF0) dependent on function
oswhndl::
        pha
        lda 0xEF        ; Check which OSWORD was called
        cmp #0xCD       ; Is it us?
        beq 1$
        pla
        rts

1$:     lda 0xF0        ; Don't know if we have to preserve this
        pha
        lda 0xF1
        pha
        jsr atapipkt    ; Issue the command and transfer data
        tax
        pla
        sta 0xF1
        pla
        sta 0xF0
        ldy #0          ; Return error code in first byte of control block
        stx (0xF0),y
        pla
        tya             ; Claim this service call with A = 0
        rts

; Process an ATAPI PACKET command
; Entry:
;       0xF0 = lo byte, 0xF1 = hi byte of address of command packet
;       (12 bytes), followed by the data address (another 2 bytes)
; Exit:
;       A = 0 on success, contents of error register on error
;       X, Y, 0xEF, 0xF0, 0xF1 not preserved
atapipkt:
; Select device
        nbsywait
        lda #0x00       ; Hardcode master for now
        iosta IDEHEAD
; Issue the PACKET command
        nbsywait
        ldx #0x00
        txa
        iosta IDEFEAT   ; No overlap or DMA
        iosta IDESCCT   ; No TCQ
        dex
        txa
        iosta IDECYLH   ; Max byte count per DRQ of 0xFFFE
        dex
        iosta IDECYLL
        lda #0xA0       ; PACKET command
        iosta IDECMD
; Output command packet
        nbsywait
        iolda IDESTAT   ; DRQ and not ERR?
        and #0x09
        cmp #0x08
        bne 3$
        ldx #0          ; Set pointer and count for command packet
        stx 0xEF
        ldx #12
        ldy #0
        jsr wrtlp
; Input/output data
        lda (0xF0),y    ; Set up data address
        pha
        iny
        lda (0xF0),y
        sta 0xF1
        pla
        sta 0xF0
1$:     nbsywait
        iolda IDESTAT   ; DRQ and not ERR?
        and #0x09
        cmp #0x08
        bne 3$
        iolda IDECYLL   ; Set up byte count for this DRQ
        tax
        iolda IDECYLH
        sta 0xEF
        iolda IDESCCT   ; Read or write?
        ror
        ror
        bcc 2$
        jsr readlp
        jmp 1$          ; Go back for next DRQ
2$:     jsr wrtlp
        jmp 1$          ; Go back for next DRQ
; Completion
3$:     iolda IDESTAT   ; ERR/CHK flag? If so, read error register
        and #0x01
        beq 4$
        iolda IDEERR
4$:     rts

; DRQ read loop
; Entry:
;       0xF0 = lo byte, 0xF1 = hi byte of data address
;       X = lo byte, 0xEF = hi byte of byte count
;       Y = offset, generally 0
; Exit:
;       0xF0 = lo byte, 0xF1 = hi byte of addr immediately after data
;       A = X = 0xEF = 0
;       Y preserved
readlp: iolda IDEDATL   ; Read low byte (high byte is latched)
        sta (0xF0),y
        inc 0xF0        ; Increment pointer
        bne 1$
        inc 0xF1
1$:     dex             ; Decrement count
        bne 2$          ; Check if finished
        lda 0xEF
        beq 7$
2$:     cpx #0xFF       ; Check if borrow
        bne 3$
        dec 0xEF

3$:     iolda IDEDATH   ; Read high byte from latch
        sta (0xF0),y
        inc 0xF0        ; Increment pointer
        bne 4$
        inc 0xF1
4$:     dex             ; Decrement count
        bne 5$          ; Check if finished
        lda 0xEF
        beq 7$
5$:     cpx #0xFF       ; Check if borrow
        bne 6$
        dec 0xEF

6$:     jmp readlp      ; Repeat until count hits zero
7$:     rts

; DRQ write loop
; Entry:
;       0xF0 = lo byte, 0xF1 = hi byte of data address
;       X = lo byte, 0xEF = hi byte of byte count
;       Y = offset, generally 0
; Exit:
;       0xF0 = lo byte, 0xF1 = hi byte of addr immediately after data
;       A = X = 0xEF = 0
;       Y preserved
wrtlp:  cpx #1          ; If count = 0x0001 here, we had an odd count
        bne 99$
        lda 0xEF
        beq 98$
99$:    iny             ; Write high byte to latch
        lda (0xF0),y
        iosta IDEDATH
        dey             ; Write low byte (word is sent to drive)
98$:    lda (0xF0),y
        iosta IDEDATL

        inc 0xF0        ; Increment pointer
        bne 1$
        inc 0xF1
1$:     dex             ; Decrement count
        bne 2$          ; Check if finished
        lda 0xEF
        beq 7$
2$:     cpx #0xFF       ; Check if borrow
        bne 3$
        dec 0xEF

3$:     inc 0xF0        ; Increment pointer
        bne 4$
        inc 0xF1
4$:     dex             ; Decrement count
        bne 5$          ; Check if finished
        lda 0xEF
        beq 7$
5$:     cpx #0xFF       ; Check if borrow
        bne 6$
        dec 0xEF

6$:     jmp wrtlp       ; Repeat until count hits zero
7$:     rts
