# labwork.S
  # Written 2015-2017 by F Lundevall
  # Skeleton file for IS1200/IS1500 lab 1.
  # The macros PUSH and POP are in the public domain.
  # Please add your own code at the end of the file.
.global delay
.global time2string
.global enable_interrupt
.global hexasc

.text
  #
  # Please keep the two macros PUSH and POP unchanged
  #
.macro        PUSH reg
        addi        $sp,$sp,-4
        sw        \reg,0($sp)
   
.endm


.macro        POP reg
        lw        \reg,0($sp)
    
        addi        $sp,$sp,4
.endm
  #
  # Please add your own code below this line
  #


hexasc:
    andi $a0, $a0, 0xF # andar a0 och 15.
    li $t0, 0xA # sätter t0 till 10.
    ble $t0, $a0, bokst #jämför om a0 är större än 10 (0xA), isåfall hoppa till hex.
    nop
    addi $v0, $a0, 0x30 # till noll i ACSSCI
    jr $ra
    nop
    
bokst:
    addi $v0, $a0, 0x37 #till A i Ascii
    jr $ra
    nop


         
    
delay:
    
    li $t3, 4711666    #loadar 4711
    li $t2, 0
    
    while:
        ble $a0,0 , sluta     #while(ms>0) -> branchar om ms är mindre än 0 till sluta.
        nop
        sub $a0, $a0, 1     # ms = ms - 1
    
        for:
            beq $t2, $t3, while     #om i = 4711, branch till while.
            nop
            addi $t2,$t2, 1     # i = i + 1
            j for
            nop
    
    
sluta:
    
    jr $ra
    nop




time2string:
    PUSH $s1
    PUSH $s2           #Sparar i stacken. (vi pushar ner on stack)
    PUSH $ra           #$ra är returadress som vi sparar i stack
    move    $s1,$a0
    move    $s2,$a1
    
    andi    $t0, $s2, 0x000F       #maskar 3an #00:0F
    srl    $a0, $t0, 0        #Sätter in t0 i a0 (adressen)
    jal     hexasc
    nop
    move    $a2,$v0
    sb    $a2, 4($s1)
    
    andi    $t0, $s2, 0x00F0    #Maskar #00:F0
    srl    $a0, $t0, 4        #Shiftar >> med 4 så vi hamnar på de 4lsb.
    jal    hexasc            #UPPGFIT3.4
    nop
    move    $a2,$v0
    sb    $a2, 3($s1)
    
    
    li    $t0, 0x3a        #: i ASCII
    sb    $t0, 2($s1)
    
    
    andi     $t0, $s2, 0x0F00    #Maskar #0F:00
    srl      $a0, $t0, 8        #Shiftar >> med 8 så vi hamnar på de 4lsb.
    jal     hexasc            #Jump till hexasc
    nop
    move     $a2,$v0
    sb     $a2,1($s1)
    
    andi     $t0, $s2, 0xF000    #maskar #F0:00
    srl      $a0, $t0, 12        #Shiftar >> med 12 så vi hamnar på de 4lsb
    jal     hexasc
    nop
    move     $a2,$v0            #Vi adderar v0 (value från hexasc) in i s3.
    sb     $a2, 0($s1)
    
    
    li     $t0, 0x0        #Null i ASCII
    sb     $t0, 5($s1)


        POP $ra
        POP $s2
        POP $s1
    
    jr $ra
    nop

enable_interrupt:
    ei
    
    jr $ra
    nop
    


