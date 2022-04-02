.text
.globl memset
.align 4

memset:
    mv t4, a0
    mv t5, a1
    mv t6, a2
    add t6, t6, t4

1:
    sb t5, 0(t4)
    addi t4, t4, 1
    blt t4, t6, 1b
    
    ret


.end
