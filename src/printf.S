.globl printf
printf:
    addiu   $sp, $sp, -0x0018
    sw      $ra, 0x0014($sp)
    sw      $a0, 0x0018($sp)
    sw      $a1, 0x001C($sp)
    sw      $a2, 0x0020($sp)
    sw      $a3, 0x0024($sp)
    addiu   $a3, $sp, 0x001C
    move    $a2, $a0
    lw      $a1, printf_str
    la      $a0, sprintf + (0x8032255C-0x803224F0)
    sw      $a1, 0x0010($sp)
    jal     ultra_80326260
    lw      $3, 0x0010($sp)
    bltz    $v0, 0f
    addu    $3, $3, $v0
    sb      $0, 0x0000($3)
    addiu   $3, $3, -0x0001
    sw      $3, printf_str
0:
    lw      $ra, 0x0014($sp)
    addiu   $sp, $sp, 0x0018
    jr      $ra

.rdata

.globl printf_buf
printf_buf: .space 2048
