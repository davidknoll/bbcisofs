        .export _exit
        .export __STARTUP__ : absolute = 1      ; Mark as startup
        .import zerobss
        .import initlib, donelib
        .import _language, _service
        .import __STACKSTART__                  ; Linker generated

        .importzp sp, sreg
        .include "zeropage.inc"

; Sideways ROM header
        .segment "STARTUP"
        jmp langent                             ; Language entry
        jmp svcent                              ; Service entry
        .byte $82                               ; Type (service, 6502)
        .byte <copy-1                           ; Copyright string pointer
        .byte $00                               ; Binary version number (shown in *ROMS)
title:  .asciiz "CD-ROM Filing System"          ; Title string (shown in *ROMS and on entering language)
copy:   .asciiz "(C) David Knoll"               ; Copyright string (no version string)

; Language entry
langent:
        php                                     ; Save all register data for the use of the C code
        sei
        cld
        sta regsv+0
        stx regsv+1
        sty regsv+2
        pla
        sta regsv+3
        tsx                                     ; Save SP in case of abnormal exit
        stx spsave
        lda #<regsv                             ; language(regsv);
        ldx #>regsv
        jsr _language
        lda regsv+3                             ; Set up returned register state
        pha
        ldy regsv+2
        ldx regsv+1
        lda regsv+0
        plp
        rts

; Service entry
svcent: php                                     ; Save all register data for the use of the C code
        sei
        cld
        sta regsv+0
        stx regsv+1
        sty regsv+2
        cmp #1                                  ; This service call happens on BREAK...
        bne :+
        jsr init                                ; ...so we do our own initialisation at that point
:       pla
        sta regsv+3
        tsx                                     ; Save SP in case of abnormal exit
        stx spsave
        lda #<regsv                             ; service(regsv);
        ldx #>regsv
        jsr _service
        lda regsv+3                             ; Set up returned register state
        pha
        ldy regsv+2
        ldx regsv+1
        lda regsv+0
        plp
        rts

; This bit basically copied from cc65's own crt0
init:   lda #<__STACKSTART__
        ldx #>__STACKSTART__
        sta sp
        stx sp+1
        jsr zerobss
        jsr initlib
        rts

; Seems a weird thing to do from the ROM, but anyway
_exit:  ldx spsave
        txs
        jsr donelib
        lda regsv+3                             ; Set up returned register state
        pha
        ldy regsv+2
        ldx regsv+1
        lda regsv+0
        plp
        rts

        .segment "DATA"
spsave: .res 1                                  ; Save 6502 SP on entry
regsv:  .res 6                                  ; Size of a struct regs
