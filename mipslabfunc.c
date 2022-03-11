/* mipslabfunc.c
   This file written 2015 by F Lundevall
   Some parts are original code written by Axel Isaksson
   For copyright and licensing, see file COPYING */

#include <stdint.h>   /* Declarations of uint_32 and the like */
#include <pic32mx.h>  /* Declarations of system-specific addresses etc */
#include "mipslab.h"  /* Declatations for these labs */

/* Declare a helper function which is local to this file */
static void num32asc( char * s, int );

#define DISPLAY_CHANGE_TO_COMMAND_MODE (PORTFCLR = 0x10)
#define DISPLAY_CHANGE_TO_DATA_MODE (PORTFSET = 0x10)

#define DISPLAY_ACTIVATE_RESET (PORTGCLR = 0x200)
#define DISPLAY_DO_NOT_RESET (PORTGSET = 0x200)

#define DISPLAY_ACTIVATE_VDD (PORTFCLR = 0x40)
#define DISPLAY_ACTIVATE_VBAT (PORTFCLR = 0x20)

#define DISPLAY_TURN_OFF_VDD (PORTFSET = 0x40)
#define DISPLAY_TURN_OFF_VBAT (PORTFSET = 0x20)

//Alla variabler globala.
uint8_t pixel[32][128];
uint8_t snake[32][128];
const uint8_t maxbredd = 128; //128 p bred
const uint8_t maxhojd = 32; //32 p hög
uint8_t direction;
uint8_t x = 16;
uint8_t y = 64;
uint8_t pixel[32][128];
uint8_t snake[32][128];
int in_startscreen = 1;
int in_game = 1;
int game_over = 0;
int score = -1;

/* quicksleep:
   A simple function to create a small delay.
   Very inefficient use of computing resources,
   but very handy in some special cases. */
void quicksleep(int cyc) {
    int i;
    for(i = cyc; i > 0; i--);
}

/* tick:
   Add 1 to time in memory, at location pointed to by parameter.
   Time is stored as 4 pairs of 2 NBCD-digits.
   1st pair (most significant byte) counts days.
   2nd pair counts hours.
   3rd pair counts minutes.
   4th pair (least significant byte) counts seconds.
   In most labs, only the 3rd and 4th pairs are used. */
void tick( unsigned int * timep ) {
  /* Get current value, store locally */
  register unsigned int t = * timep;
  t += 1; /* Increment local copy */

  /* If result was not a valid BCD-coded time, adjust now */

  if( (t & 0x0000000f) >= 0x0000000a ) t += 0x00000006;
  if( (t & 0x000000f0) >= 0x00000060 ) t += 0x000000a0;
  /* Seconds are now OK */

  if( (t & 0x00000f00) >= 0x00000a00 ) t += 0x00000600;
  if( (t & 0x0000f000) >= 0x00006000 ) t += 0x0000a000;
  /* Minutes are now OK */

  if( (t & 0x000f0000) >= 0x000a0000 ) t += 0x00060000;
  if( (t & 0x00ff0000) >= 0x00240000 ) t += 0x00dc0000;
  /* Hours are now OK */

  if( (t & 0x0f000000) >= 0x0a000000 ) t += 0x06000000;
  if( (t & 0xf0000000) >= 0xa0000000 ) t = 0;
  /* Days are now OK */

  * timep = t; /* Store new value */
}

/* display_debug
   A function to help debugging.
   After calling display_debug,
   the two middle lines of the display show
   an address and its current contents.
   There's one parameter: the address to read and display.
   Note: When you use this function, you should comment out any
   repeated calls to display_image; display_image overwrites
   about half of the digits shown by display_debug.
*/
void display_debug( volatile int * const addr ) {
  display_string( 1, "Addr" );
  display_string( 2, "Data" );
  num32asc( &textbuffer[1][6], (int) addr );
  num32asc( &textbuffer[2][6], *addr );
  display_update_string();
}

uint8_t spi_send_recv(uint8_t data) {
    while(!(SPI2STAT & 0x08));
    SPI2BUF = data;
    while(!(SPI2STAT & 1));
    return SPI2BUF;
}
void display_init(void) {
        DISPLAY_CHANGE_TO_COMMAND_MODE;
    quicksleep(10);
    DISPLAY_ACTIVATE_VDD;
    quicksleep(1000000);

    spi_send_recv(0xAE);
    DISPLAY_ACTIVATE_RESET;
    quicksleep(10);
    DISPLAY_DO_NOT_RESET;
    quicksleep(10);

    spi_send_recv(0x8D);
    spi_send_recv(0x14);

    spi_send_recv(0xD9);
    spi_send_recv(0xF1);

    DISPLAY_ACTIVATE_VBAT;
    quicksleep(10000000);

    spi_send_recv(0xA1);
    spi_send_recv(0xC8);

    spi_send_recv(0xDA);
    spi_send_recv(0x20);

    spi_send_recv(0xAF);
}
void display_string(int line, char *s) {
    int i;
    if(line < 0 || line >= 4)
        return;
    if(!s)
        return;

    for(i = 0; i < 16; i++)
        if(*s) {
            textbuffer[line][i] = *s;
            s++;
        } else
            textbuffer[line][i] = ' ';
}
void display_image(int x, const uint8_t *data) {
    int i, j;

    for(i = 0; i < 4; i++) {
        DISPLAY_CHANGE_TO_COMMAND_MODE;

        spi_send_recv(0x22);
        spi_send_recv(i);

        spi_send_recv(x & 0xF);
        spi_send_recv(0x10 | ((x >> 4) & 0xF));

        DISPLAY_CHANGE_TO_DATA_MODE;

        for(j = 0; j < 32; j++)
            spi_send_recv(~data[i*32 + j]);
    }
}
void display_update_screen(){
    int a, b, d, v; // column/page/pagerow, värdet
    for(b = 0; b < 4; b++){
        DISPLAY_CHANGE_TO_COMMAND_MODE;
        spi_send_recv(0x22);
        spi_send_recv(b);

        spi_send_recv(0x00);
        spi_send_recv(0x10);

        DISPLAY_CHANGE_TO_DATA_MODE;

        for(a = 0; a < 128; a++){
            v = (pixel[b * 8][a]); //page * 8 lägsta bit page row 7:0
            for(d = 1; d < 8; d++){
                //sätter pixel värdet från lägsta till biten i byte till högsta
                v |= (pixel[b * 8 + d][a]) << d;
            }
            
            spi_send_recv(v);
        }
    }
}
void display_update_string(void) {
    int i, j, k;
    int c;
    for(i = 0; i < 4; i++) {
        DISPLAY_CHANGE_TO_COMMAND_MODE;
        spi_send_recv(0x22);
        spi_send_recv(i);

        spi_send_recv(0x0);
        spi_send_recv(0x10);

        DISPLAY_CHANGE_TO_DATA_MODE;

        for(j = 0; j < 16; j++) {
            c = textbuffer[i][j];
            if(c & 0x80)
                continue;

            for(k = 0; k < 8; k++)
                spi_send_recv(font[c*8 + k]);
        }
    }
}

/* Helper function, local to this file.
   Converts a number to hexadecimal ASCII digits. */
static void num32asc( char * s, int n ) {
  int i;
  for( i = 28; i >= 0; i -= 4 )
    *s++ = "0123456789ABCDEF"[ (n >> i) & 15 ];
}

/*
 * nextprime
 *
 * Return the first prime number larger than the integer
 * given as a parameter. The integer must be positive.
 */
#define PRIME_FALSE   0     /* Constant to help readability. */
#define PRIME_TRUE    1     /* Constant to help readability. */

int nextprime( int inval ) {
   register int perhapsprime = 0; /* Holds a tentative prime while we check it. */
   register int testfactor; /* Holds various factors for which we test perhapsprime. */
   register int found;      /* Flag, false until we find a prime. */

   if (inval < 3 )          /* Initial sanity check of parameter. */
   {
     if(inval <= 0) return(1);  /* Return 1 for zero or negative input. */
     if(inval == 1) return(2);  /* Easy special case. */
     if(inval == 2) return(3);  /* Easy special case. */
   }
   else
   {
     /* Testing an even number for primeness is pointless, since
      * all even numbers are divisible by 2. Therefore, we make sure
      * that perhapsprime is larger than the parameter, and odd. */
     perhapsprime = ( inval + 1 ) | 1 ;
   }
   /* While prime not found, loop. */
   for( found = PRIME_FALSE; found != PRIME_TRUE; perhapsprime += 2 )
   {
     /* Check factors from 3 up to perhapsprime/2. */
     for( testfactor = 3; testfactor <= (perhapsprime >> 1) + 1; testfactor += 1 )
     {
       found = PRIME_TRUE;      /* Assume we will find a prime. */
       if( (perhapsprime % testfactor) == 0 ) /* If testfactor divides perhapsprime... */
       {
         found = PRIME_FALSE;   /* ...then, perhapsprime was non-prime. */
         goto check_next_prime; /* Break the inner loop, go test a new perhapsprime. */
       }
     }
     check_next_prime:;         /* This label is used to break the inner loop. */
     if( found == PRIME_TRUE )  /* If the loop ended normally, we found a prime. */
     {
       return( perhapsprime );  /* Return the prime we found. */
     }
   }
   return( perhapsprime );      /* When the loop ends, perhapsprime is a real prime. */
}

/*
 * itoa
 *
 * Simple conversion routine
 * Converts binary to decimal numbers
 * Returns pointer to (static) char array
 *
 * The integer argument is converted to a string
 * of digits representing the integer in decimal format.
 * The integer is considered signed, and a minus-sign
 * precedes the string of digits if the number is
 * negative.
 *
 * This routine will return a varying number of digits, from
 * one digit (for integers in the range 0 through 9) and up to
 * 10 digits and a leading minus-sign (for the largest negative
 * 32-bit integers).
 *
 * If the integer has the special value
 * 100000...0 (that's 31 zeros), the number cannot be
 * negated. We check for this, and treat this as a special case.
 * If the integer has any other value, the sign is saved separately.
 *
 * If the integer is negative, it is then converted to
 * its positive counterpart. We then use the positive
 * absolute value for conversion.
 *
 * Conversion produces the least-significant digits first,
 * which is the reverse of the order in which we wish to
 * print the digits. We therefore store all digits in a buffer,
 * in ASCII form.
 *
 * To avoid a separate step for reversing the contents of the buffer,
 * the buffer is initialized with an end-of-string marker at the
 * very end of the buffer. The digits produced by conversion are then
 * stored right-to-left in the buffer: starting with the position
 * immediately before the end-of-string marker and proceeding towards
 * the beginning of the buffer.
 *
 * For this to work, the buffer size must of course be big enough
 * to hold the decimal representation of the largest possible integer,
 * and the minus sign, and the trailing end-of-string marker.
 * The value 24 for ITOA_BUFSIZ was selected to allow conversion of
 * 64-bit quantities; however, the size of an int on your current compiler
 * may not allow this straight away.
 */
#define ITOA_BUFSIZ ( 24 )

char * itoaconv( int num ) {
  register int i, sign;
  static char itoa_buffer[ ITOA_BUFSIZ ];
  static const char maxneg[] = "-2147483648";

  itoa_buffer[ ITOA_BUFSIZ - 1 ] = 0;   /* Insert the end-of-string marker. */
  sign = num;                           /* Save sign. */
  if( num < 0 && num - 1 > 0 )          /* Check for most negative integer */
  {
    for( i = 0; i < sizeof( maxneg ); i += 1 )
    itoa_buffer[ i + 1 ] = maxneg[ i ];
    i = 0;
  }
  else
  {
    if( num < 0 ) num = -num;           /* Make number positive. */
    i = ITOA_BUFSIZ - 2;                /* Location for first ASCII digit. */
    do {
      itoa_buffer[ i ] = num % 10 + '0';/* Insert next digit. */
      num = num / 10;                   /* Remove digit from number. */
      i -= 1;                           /* Move index to next empty position. */
    } while( num > 0 );
    if( sign < 0 )
    {
      itoa_buffer[ i ] = '-';
      i -= 1;
    }
  }
  /* Since the loop always sets the index i to the next empty position,
   * we must add 1 in order to return a pointer to the first occupied position. */
  return( &itoa_buffer[ i + 1 ] );
}

//--------------------------------------------------------------//
//uint8_t är unsigned 8-bit integer. Bredden är exakt 8 bits, dvs sizen är 1 byte. Vi använder det pga hur skrämen är uppdelad.
void move() {
    if (direction == up) {
        x--;
    } else if (direction == right) {
        y++;
    } else if (direction == down){
        x++;
    }
      else if (direction == left){
        y--;
    }
    pixel[x][y] = 1;    //
    snake[x][y] = ((direction + 2) % 4); //modulu för att hitta den motsatta riktiningen för den sista pixeln.
 
  remove();
}


void remove() {
  uint8_t x_current = x;
  uint8_t y_current = y;
  uint8_t x_next = x_current;
  uint8_t y_next = y_current;
  uint8_t not_removed = 1;
 
  while (not_removed) {
    if (snake[x_current][y_current]  == up) {
      x_next = x_current - 1;
    }
    if (snake[x_current][y_current]== right) {
      y_next = y_current + 1;
    }
    if (snake[x_current][y_current]  == down) {
      x_next = x_current + 1;
    }
    if (snake[x_current][y_current]  == left) {
      y_next = y_current - 1;
    }
    if (snake[x_next][y_next] == null) { 
      snake[x_current][y_current] = null; //Ny svans
      pixel[x_next][y_next] = 0;    //tar bort med att sätta pixeln 0
      not_removed = 0;
    }
    else {
      x_current = x_next;
      y_current = y_next;
    }
  }
}

//rand() genererar number mellan 0 till randmax. För att få olika generationer så callar vi på srand();
int random1(int min, int max) {
  
    return (rand() % (max));
}

void food1() {
    int no_food = 1;

    while (no_food) {
        int ran_x = random1(maxbredd, (maxhojd));
        int ran_y = random1(maxhojd, (maxbredd));
        
        if (pixel[ran_x][ran_y] == 0) { //om det ej är någon mat dvs pixel == 1 (ormen äter) så sätter pixel till 1 och food.
            pixel[ran_x][ran_y] = 1;
            snake[ran_x][ran_y] = food;
            no_food = 0;
            score++;
        }
    }
}

void eat() {
    if (direction == up) {
        x--;
    } else if (direction == right) {
        y++;
    } else if (direction == down){
        x++;
    } else if (direction == left){
        y--;
    }
    pixel[x][y] = 1;
    snake[x][y] = ((direction + 2) % 4);
    food1();
}

void gameground() {
    int i = 0;
    int j = 0;
    in_game = 1;
    game_over = 0;
    score = -1; //då den åker in i väggen + 1.
  
  for (i = 0; i < maxhojd; i++) {   //två loopar går igenom höjden & bredden
    for (j = 0; j < maxbredd; j++) {
      snake[i][j] = null; // riktningen för föregående pixel till null
      if (i == 0 || (i == (maxhojd - 1)) || (j == 0) || (j == (maxbredd - 1))) { //ramen som vi sätter till 1 och namnet wall.
        pixel[i][j] = 1;
        snake[i][j] = wall;
      }
      else{
        pixel[i][j] = 0; // allt utom ramen ska vara 0 dvs svart
      }
    }
  }
  pixel[x][y] = 1; // ormen
    food1();
}

 void snakE() {
    uint8_t next_x = x;
    uint8_t next_y = y;
    uint8_t next_snake;
    
     if (direction == up) {
        next_x = x - 1;
    } else if (direction == right) {
        next_y = y + 1;
    } else if (direction == down){
        next_x = x + 1;
    } else if (direction == left) {
        next_y = y - 1;
    }
    next_snake = snake[next_x][next_y];
    if (pixel[next_x][next_y] == 1) { //om nästa steg är 1 dvs någon pixel som mat, orm eller väggen.
        if (next_snake == wall) { //om nästa är väggen då sätter vi ingame=0 dvs ej i spelet längre
            in_game = 0;
        }
         else if (next_snake == food){ //om nästa är mat så hoppa in i funktionen eat.
            eat();
        }
     else if(next_snake >=0 && next_snake <= 2) { //om nästa är 1 så är det ormen.
        in_game = 0;
        }
    }
    else {
        move();
    }
     
}

void startscreen() {
    display_string(0, "     SnakE");
    display_string(1, "  av em & ol");
    display_string(2, " btn to play");
    display_string(3, " e=sw1,h=sw2");
    display_update_string();
    display_image(96,icon);
}
void gameoverscreen() {
    display_string(0, "GAME OVER");
    display_string(1, "Your score");
    display_string(2,itoaconv(score));
    display_string(3, "try igen");
    display_update_string();
    display_image(96,icon);
}
