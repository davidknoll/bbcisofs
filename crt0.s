        .export _exit
        .export __STARTUP__ : absolute = 1      ; Mark as startup
        .import zerobss
        .import initlib, donelib
        .import _service
        .import __STACKSTART__                  ; Linker generated

        .export _osfile_entry, _osargs_entry, _osbget_entry, _osbput_entry
        .export _osgbpb_entry, _osfind_entry, _osfsc_entry
        .import _osfile_handler, _osargs_handler, _osbget_handler, _osbput_handler
        .import _osgbpb_handler, _osfind_handler, _osfsc_handler
        .export _secbuf

        .importzp sp, sreg
        .include "zeropage.inc"

; Sideways ROM header
        .segment "STARTUP"
        .byte 0, 0, 0                           ; No language entry
        jmp svcent                              ; Service entry
        .byte $82                               ; Type (service, 6502)
        .byte <copy-1                           ; Copyright string pointer
        .byte $00                               ; Binary version number (shown in *ROMS)
title:  .asciiz "CD-ROM Filing System"          ; Title string (shown in *ROMS and on entering language)
copy:   .asciiz "(C) David Knoll"               ; Copyright string (no version string)

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

_osfile_entry:
        sty sreg
        ldy #0
        sty sreg+1
        jsr _osfile_handler
        ldy sreg
        rts

_osargs_entry:
        sty sreg
        ldy #0
        sty sreg+1
        jsr _osargs_handler
        ldy sreg
        rts

_osbget_entry:
        sty sreg
        ldy #0
        sty sreg+1
        jsr _osbget_handler
        ldy sreg
        rts

_osbput_entry:
        sty sreg
        ldy #0
        sty sreg+1
        jsr _osbput_handler
        ldy sreg
        rts

_osgbpb_entry:
        sty sreg
        ldy #0
        sty sreg+1
        jsr _osgbpb_handler
        ldy sreg
        rts

_osfind_entry:
        sty sreg
        ldy #0
        sty sreg+1
        jsr _osfind_handler
        ldy sreg
        rts

_osfsc_entry:
        sty sreg
        ldy #0
        sty sreg+1
        jsr _osfsc_handler
        ldy sreg
        rts

        .segment "DATA"
spsave: .res 1                                  ; Save 6502 SP on entry
regsv:  .res 6                                  ; Size of a struct regs

        .segment "SHARED"
_secbuf: .res 2048                              ; Directory sector buffer
