        .title ISO-9660 (CDFS) for the BBC Micro
        .module header
        .r6500
        .area SWROM

; Sideways ROM header
header::
        .db 0x00,0x00,0x00              ; No language entry
        jmp svcent                      ; Service entry
        .db 0x82                        ; Type- service, 6502
        .db <cpmsg - 1                  ; Copyright message pointer
        .db 0x00                        ; Binary version number

tlmsg:: .strz "ISO-9660 (CDFS)"         ; Title string
vrmsg:: .strz "0.01 (16 Jan 2017)"      ; Version string
cpmsg:: .strz "(C)2017 David Knoll"     ; Copyright string

; Service entry point
svcent::
        cmp #0x02                       ; Claim private workspace in RAM
        beq 2$
        cmp #0x08                       ; OSWORD not recognised
        beq 8$
        cmp #0x09                       ; *HELP issued
        beq 9$
        rts
2$:     jmp claimprv
8$:     jmp oswhndl
9$:     jmp starhelp

; Claim private workspace
claimprv::
        sty 0xDF0,x                     ; Store base address of my workspace
        iny                             ; Claim 1 page
        rts
