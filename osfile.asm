        .title ISO-9660 (CDFS) for the BBC Micro
        .module osfind
        .r6500
        .area SWROM

; OSFIND handler
; Vector to be installed when we are the selected FS
; Entry:
;       A = opcode (zero = close, nonzero = open)
;       If A = 0: Y = file handle or 0 for all files
;       If not: X = lo byte, Y = hi byte of pointer to filename
; Exit:
;       A preserved if called with A = 0
;       Otherwise, A = file handle, or 0 on error
;       X, Y preserved.

scandir:
        ldy #0          ; Init pointer to sector buffer
        sty cp_mem
        ldx 0xF4
        ldy 0xDF0,x
        iny
        sty cp_mem+1

        ldy #32         ; Length of this filename
        lda [cp_mem],y
        iny
        tax
        beq found

1$:     lda [cp_mem],y  ; Byte of directory filename
        cmp [fn_ptr],y  ; Compare to requested filename
        bne 2$
        iny             ; Go and check next byte
        jmp 1$
2$:     cmp #';'        ; End of directory filename?
        bne 3$          ; We know this one's not a match
        lda [fn_ptr],y  ; End of requested filename?
        cmp #0x0D
        beq found       ; At this point we've found it
3$:; Advance to next directory entry

        cmp #';'
        bne ...
        lda [fn_ptr],y  ; Compare to requested filename
        cmp #0x0D
        beq found       ; Positive match
        jmp nextdir     ; Negative match


findpvd:
        lda #0          ; VDs start at sector 0x10
        sta cp_lba
        sta cp_lba+1
        sta cp_lba+2
        lda #0x10
        sta cp_lba+3
1$:     jsr readsect
        lda cp_op       ; Error?
        bne 3$
        ldy #0          ; Have we found a PVD?
        lda [cp_mem],y
        cmp #1          ; Yes
        beq 2$
        cmp #255        ; No, and we've found the terminator
        beq 3$
        inc cp_lba+3    ; Next sector then
        bne 1$
        inc cp_lba+2
        bne 1$
        inc cp_lba+1
        bne 1$
        inc cp_lba
        jmp 1$
2$:     tya             ; Return A=0 on success
3$:     and #0xFF
        rts

readsect:
        ldy #0x28       ; Set up command packet
        sty cp_op
        ldy #0
        sty cp_cnt
        sty cp_mem
        iny
        sty cp_cnt+1
        ldx 0xF4        ; ie. load will start at 1 page
        ldy 0xDF0,x     ; into our private storage
        iny
        sty cp_mem+1
        lda #0xCD       ; Issue command
        ldx #<cmdpkt
        ldy #>cmdpkt
        jmp OSWORD

        .area ZP (ABS)
        .org 0xB0
cmdpkt:
cp_op:  .ds 1           ; Packet command opcode
        .ds 1
cp_lba: .ds 4           ; Sector LBA, big-endian
        .ds 1
cp_cnt: .ds 2           ; Sector count, big-endian
        .ds 3
cp_mem: .ds 2           ; Data address, little-endian
fn_ptr: .ds 2           ; Pointer to requested filename
