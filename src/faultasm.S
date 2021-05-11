#include "fault.h"

.set noreorder
.set noat

.globl fault_darken
fault_darken:
    li      $v0, 0x39CE39CE
    li      $v1, 0x00010001
    li      $a2, VIDEO_W*VIDEO_H/2
0:
    lw      $at, ($a1)
    srl     $at, $at, 2
    and     $at, $at, $v0
    or      $at, $at, $v1
    sw      $at, ($a0)
    addiu   $a2, $a2, -1
    addiu   $a0, $a0, 4
    bnez    $a2, 0b
    addiu   $a1, $a1, 4
    jr      $ra
    nop

.globl fault_print
fault_print:
    lui     $9, 0x8000
0:
    lbu     $2, ($a1)
    bnezl   $2, 1f
    ori     $1, $0, ' '
    jr      $ra
    nop
1:
    beql    $2, $1, 4f
    addiu   $a0, $a0, 2*6
    ori     $1, $0, ':'
    beql    $2, $1, 1f
    ori     $2, $0, 0x24
    ori     $1, $0, '('
    beql    $2, $1, 1f
    ori     $2, $0, 0x25
    ori     $1, $0, ')'
    beql    $2, $1, 1f
    ori     $2, $0, 0x26
    ori     $1, $0, '-'
    beql    $2, $1, 1f
    ori     $2, $0, 0x27
    ori     $1, $0, '+'
    beql    $2, $1, 1f
    ori     $2, $0, 0x2A
    ori     $1, $0, '.'
    beql    $2, $1, 1f
    ori     $2, $0, 0x2C
    slt     $1, $2, '9'+1
    bnezl   $1, 1f
    addiu   $2, $2, 0x00-'0'
    slt     $1, $2, 'Z'+1
    bnezl   $1, 1f
    addiu   $2, $2, 0x0A-'A'
    addiu   $2, $2, 0x0A-'a'
1:
    ori     $1, $0, 5
    divu    $2, $1
    mflo    $2
    sll     $1, $2, 3
    subu    $2, $1, $2
    sll     $2, $2, 2
    lui     $1, %hi(font)
    addu    $2, $2, $1
    mfhi    $3
    sll     $1, $3, 1
    addu    $3, $3, $1
    sll     $3, $3, 1
    ori     $6, $0, 7
1:
    lw      $8, %lo(font)($2)
    sllv    $8, $8, $3
    addiu   $2, $2, 4
    ori     $a3, $0, 6
2:
    and     $1, $8, $9
    bnezl   $1, 3f
    ori     $1, $1, 0xFFFF
    ori     $1, $1, 0x0001
3:
    sh      $1, ($a0)
    sll     $8, $8, 1
    addiu   $7, $7, -1
    bnez    $7, 2b
    addiu   $a0, $a0, 2
    addiu   $6, $6, -1
    bnezl   $6, 1b
    addiu   $a0, $a0, 2*(320-6)
    addiu   $a0, $a0, -2*(320*6)
4:
    b       0b
    addiu   $a1, $a1, 1

.set reorder
.set at

.globl fault_printf
fault_printf:
    sw      $ra, fault_ra
    sw      $a0, fault_a0
    la      $a0, fault_buf
    jal     sprintf
    lw      $ra, fault_ra
    lw      $a0, fault_a0
    la      $a1, fault_buf
    j       fault_print

.rdata

font:
    .word 0x70871C30
    .word 0x8988A250
    .word 0x88808290
    .word 0x88831C90
    .word 0x888402F8
    .word 0x88882210
    .word 0x71CF9C10
    .word 0xF9CF9C70
    .word 0x8228A288
    .word 0xF200A288
    .word 0x0BC11C78
    .word 0x0A222208
    .word 0x8A222288
    .word 0x71C21C70
    .word 0x23C738F8
    .word 0x5228A480
    .word 0x8A282280
    .word 0x8BC822F0
    .word 0xFA282280
    .word 0x8A28A480
    .word 0x8BC738F8
    .word 0xF9C89C08
    .word 0x82288808
    .word 0x82088808
    .word 0xF2EF8808
    .word 0x82288888
    .word 0x82288888
    .word 0x81C89C70
    .word 0x8A08A270
    .word 0x920DA288
    .word 0xA20AB288
    .word 0xC20AAA88
    .word 0xA208A688
    .word 0x9208A288
    .word 0x8BE8A270
    .word 0xF1CF1CF8
    .word 0x8A28A220
    .word 0x8A28A020
    .word 0xF22F1C20
    .word 0x82AA0220
    .word 0x82492220
    .word 0x81A89C20
    .word 0x8A28A288
    .word 0x8A28A288
    .word 0x8A289488
    .word 0x8A2A8850
    .word 0x894A9420
    .word 0x894AA220
    .word 0x70852220
    .word 0xF8011000
    .word 0x08020800
    .word 0x10840400
    .word 0x20040470
    .word 0x40840400
    .word 0x80020800
    .word 0xF8011000
    .word 0x70800000
    .word 0x88822200
    .word 0x08820400
    .word 0x108F8800
    .word 0x20821000
    .word 0x00022200
    .word 0x20800020

.bss

fault_buf:  .space 64
fault_ra:   .space 4
fault_a0:   .space 4
