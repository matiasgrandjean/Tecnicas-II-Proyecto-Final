.text
.arm
.global map

map:
    stmfd sp!,    {r4}
    ldr     r4, [sp, #4]
    sub     r0, r0, r1
    sub     r4, r4, r3
    sub     r2, r2, r1
    mul     r0, r0, r4
    sdiv    r0, r0, r2
    add     r0, r0, r3

    ldmfd sp!,     {r4}
    mov pc, lr
    
