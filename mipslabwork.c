/* mipslabwork.c
   This file written 2015 by F Lundevall
   Updated 2017-04-21 by F Lundevall
   This file should be changed by YOU! So you must
   add comment(s) here with your name(s) and date(s):
   This file modified 2017-04-31 by Ture Teknolog
   For copyright and licensing, see file COPYING */

#include <stdint.h>   /* Declarations of uint_32 and the like */
#include <pic32mx.h>  /* Declarations of system-specific addresses etc */
#include "mipslab.h"  /* Declatations for these labs */


/* Interrupt Service Routine */
void user_isr( void ) {
  if (IFS(0) & (0x100)) { //sätter på T2IF (time2interuptflag)
    IFSCLR(0) = 0x100; // clearar timeoutflag

    // button 1, ligger borta på portF
      if(PORTF & (0x1 << 1)){
          PORTFCLR = (0x1 << 1);
          if(game_over){
              game_over = 0;
          }
          if(in_startscreen){
              in_startscreen = 0;
          }
          direction = right;
      }
      
      //button 2
      if(getbtns() & 0x1){
          PORTDCLR = (0x1 <<5);
          if(game_over){
              game_over = 0;
          }
          if(in_startscreen){
              in_startscreen = 0;
          }
          direction = down; //går nedåt
      }
      
      //button 3
      if(getbtns() & 0x2){
          PORTDCLR = (0x1 <<5);
          if(game_over){
              game_over = 0;
          }
          if(in_startscreen){
              in_startscreen = 0;
          }
          direction = up;
      }
      
      //button 4
      if(getbtns() & 0x4){
          PORTDCLR = (0x1 <<5);
          if(game_over){
              game_over = 0;
          }
          if(in_startscreen){
              in_startscreen = 0;
          }
          direction = left;
    }
  }
}

/* Lab-specific initialization goes here */
void labinit( void ) {
    display_init();
    startscreen();
    
    T2CONSET = 0x70;                // Sätter prescale till 256, 0111 0000 (en mindre prescalar gör att den inte får plats i 16 bitar)
    PR2 = (80000000 / 256) / 10;    //Sätt clock period (hur högt den räknar) till klockfrekvensen/prescaler/10 sek.
    TMR2 = 0;                       //Clear timer register (i TMR2 ser man hur högt den räknat den sekunden du läser registret)
    T2CONSET= 0x8000;               // Slår på timern
    
    volatile int* trise = (volatile int*) 0xbf886100;   //Pekar på adressen
    *trise = ~0xFF;                                     //Porten som den pekar på                                                   sättter vi som 0.
    TRISD |= 0xFE0; // 1111 1110 0000 Sätt 11-5 till output.
    TRISFSET = (0x1 << 1); // set knapp 1
    //TRISE = ~0xFF;
   // PORTECLR = 0xFF;
   // TRISDSET = 0xFE0;
    
   
    
    
    IEC(0) = IEC(0) | 0x100; //Sätter på interrupts för timer 2
    IPC(2) = IPC(2) | 0x1F;  //Sätter interrupt priority till högsta (för föreläsning 6 tyckte det var bra)
    IEC(0) = IEC(0) | 0x8000;   //Sätter på interrupts 2 för swtichen 3.
   // IPC(3) = IPC(3) | 0x1F;
    enable_interrupt(); //roppar på metod i labwork.s
  return;
}
/* This function is called repetitively from the main program */
void labwork( void ) {
    //om både sw1 och sw2 nedtrckt så paus.
    if(PORTD & (0x1 << 9)){ //om sw2 är nertryckt så lvl 2
        PORTDCLR = (0x1 << 9);
        display_update_screen();
        snakE();
        quicksleep(300000); //ändrar hastigheten
    }
    
    if(PORTD & (0x1 << 8)){ //om sw1 är nertryckt så lvl1
        PORTDCLR = (0x1 << 8);
        display_update_screen();
        snakE();
        quicksleep(800000); //ändrar hastigheten
    }
    
    display_update_screen();
    srand((unsigned) TMR2); //för funktionen random1.tmr2 counter
}

