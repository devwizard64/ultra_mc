.globl __floatundisf
__floatundisf:
    dsll32  $2, $a0, 0
    or      $2, $2, $a1
    dmtc1   $2, $f0
    cvt.s.l $f0, $f0
    jr      $ra
