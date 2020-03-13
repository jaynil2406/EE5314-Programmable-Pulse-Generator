uint32_t power(uint32_t x , uint32_t p)
{
    __asm("              MOV R0,#1");   //Place X in place of #1
    __asm("             MOV R0,[R0]");
    __asm("              BLT DONE1");
    __asm("             CBZ R0,ZERO1");
    __asm("             MOV R1,#2");    //Place P in place of #2
    __asm("             MOV R1,[R1]");
    __asm("             BLT DONE1");
    __asm("             CMP R1,#2");
    __asm("              B J");
    __asm("             CBZ R1,ZERO2");
    __asm("LOOP1:         SUB R1,#1");
    __asm("             MUL R0,R0,R0");
    __asm("             CBZ R1,DONE");
    __asm("             B LOOP1");
    __asm("ZERO1:       MOV R0,#0");
    __asm("             B DONE");
    __asm("ZERO2:        MOV R1,#1");
    __asm("             B DONE");
    __asm("DONE:        BX LR");
    __asm("J:           MOV R0,#1");
    __asm("             MUL R1,R0,R0");
    __asm("             B DONE");
    __asm("DONE1:");
    return 0;
}