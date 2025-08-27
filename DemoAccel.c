/*
 * Final Project - SNAKE
 * Student - Eliya Samary 206484628
 */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "System/system.h"
#include "System/delay.h"
#include "oledDriver/oledC.h"
#include "oledDriver/oledC_colors.h"
#include "oledDriver/oledC_shapes.h"
#include "i2cDriver/i2c1_driver.h"
#include "Accel_i2c.h"

// DEFINE ENUM & STRUCTS
typedef enum direction_ { UP, DOWN,LEFT, RIGHT } direction;
#define MAX_CHARMS 6

// SNAKE STRUCTS
struct snake_head {
    uint8_t xs, ys, xe, ye;
    struct snake_part* body;
    direction snake_d;
    uint16_t color;

};
struct snake_part {
    uint8_t xs, ys, xe, ye;
    struct snake_part* next;
    struct snake_part* prev;
    uint16_t color;
    bool display;
};
struct charm {
    uint8_t xs, ys;
    uint16_t color;
};

// GLOBAL VARIABLES
int x_offset = 0;                  // for potentiometer
direction snake_direction = RIGHT; // default
bool isGameOver = true;            // for game loop
int gameCount = 0;
int gameResult = 4;
struct snake_head head;
struct snake_part part1;
struct snake_part part2;
struct snake_part part3;
struct snake_part part4;
struct snake_part part5;
struct snake_part part6;
struct snake_part part7;
struct snake_part part8;
struct snake_part part9;
struct snake_part part10;
struct snake_part* last_part;
struct charm game_charms[MAX_CHARMS];

// FUNCTION DECLARATIONS
void errorStop(char *msg);

void init(void);

void initialize_snake_parts();

void __attribute__((__interrupt__))_T1Interrupt(void);

direction which_direction(unsigned char X, unsigned char Y);

void made_charms (struct charm * game_charms);

void snake_display(struct snake_part* body);

void charms_display(struct charm* charms_);

void snake_movement(struct snake_head* head, uint8_t temp_xs, uint8_t temp_ys, uint8_t temp_xe, uint8_t temp_ye);

void up_direction(struct snake_head* head, uint8_t* _xs, uint8_t* _ys, uint8_t* _xe, uint8_t * _ye);

void down_direction(struct snake_head* head, uint8_t* _xs, uint8_t* _ys, uint8_t* _xe, uint8_t * _ye);

void left_direction(struct snake_head* head, uint8_t* _xs, uint8_t* _ys, uint8_t* _xe, uint8_t* _ye);

void right_direction(struct snake_head* head, uint8_t* _xs, uint8_t* _ys, uint8_t* _xe, uint8_t* _ye);

void update_snake_direction(int x, int y);

bool check_snake_boundary(uint8_t temp_xs, uint8_t temp_ys);

void calculate_result();

void game_display();

void display_end_message(char* s);

/*
                         Main application
 */
int main(void) {
    unsigned char id = 0;
    I2Cerror rc;
    int x, y;
    char string[] = "     ";
    unsigned char xyz[6] = {0};

//  INITIALIZATIONS
    SYSTEM_Initialize();
    init();
    
    oledC_setBackground(OLEDC_COLOR_BLACK);
    oledC_clearScreen();

    i2c1_driver_driver_close();
    i2c1_open();

    rc = i2cReadSlaveRegister(0x3A, 0, &id);
    
    if (rc == !OK)
        errorStop("I2C Error");
    
    initialize_snake_parts();

    rc = i2cWriteSlave(0x3A, 0x2D, 8);

    made_charms(game_charms);

//   GAME LOOP
    while (isGameOver == true) {

        AD1CON1bits.SAMP = 1;  // SAMP START
        for (int i=0; i<=100; i++){} //DELAY
        AD1CON1bits.SAMP = 0;  // SAMP END
        while (AD1CON1bits.DONE) {}
        float value = ADC1BUF0 / 1024.0;
        x_offset = value * 96;

        i2cReadSlaveMultRegister(0x3A, 0x32, 6, xyz);

        x = xyz[0] + xyz[1]*256; //2xbytes ==> word
        y = xyz[2] + xyz[3]*256;

        oledC_clearScreen();
        
        game_display();
        update_snake_direction(x, y);
        calculate_result();
        
    }
    
// GAME END
    oledC_clearScreen();
    // END MESSAGE
    display_end_message(string);
}

// FUNCTIONS IMPLLEMENTATIONS
void errorStop(char *msg) {
    oledC_DrawString(0, 20, 2, 2, msg, OLEDC_COLOR_DARKRED);

    for (;;)
        ;
}

void init(void) {
    //Configure A/D Control Registers (ANSB & AD1CONx SFRs)
    ANSBbits.ANSB12 = 1;
    TRISBbits.TRISB12 = 1;
    AD1CHS = 8;
    AD1CON1 &= 0;
    AD1CON1 = AD1CON1 | (1 << 15);
    AD1CON2 &= 0;
    AD1CON3 |= 0x10FF;
    
    // Timer 1 initialize
    T1CONbits.TON = 1;      // bit 15 TON = 0
    T1CONbits.TSIDL = 1;    // bit 13 = 1 
    T1CONbits.TGATE = 0;    // bit 6 = 0 = Gated time accumulation is disabeld
    T1CONbits.TCKPS0 = 0;   //
//    T1CONbits.TCKPS1 = 1; 
    T1CONbits.TCKPS = 11;
    T1CONbits.TCS = 0;      // Internal clock (Fosc/2)
    PR1 = 0xF424;           // timer period value 4M/64
    
    // Timer 1 interrupt setup
    IEC0bits.T1IE = 1;      // enable = 1
    IFS0bits.T1IF = 0;      // flag = 0

}

void initialize_snake_parts(){

    head.xs = 20;
    head.ys = 30;
    head.xe = 22;
    head.ye = 34;
    head.color = OLEDC_COLOR_RED;
    head.body = &part1;
    head.snake_d = UP;
    part1.xs = 20;
    part1.ys = 34;
    part1.xe = 22;
    part1.ye = 38;
    part1.color = OLEDC_COLOR_GREEN;
    part1.display = true;
    part1.next = &part2;
    part1.prev = NULL;
    part2.xs = 20;
    part2.ys = 38;
    part2.xe = 22;
    part2.ye = 42;
    part2.color = OLEDC_COLOR_BLUE;
    part2.display = true;
    part2.next = &part3;
    part2.prev = &part1;
    part3.xs = 20;
    part3.ys = 42;
    part3.xe = 22;
    part3.ye = 46;
    part3.color = OLEDC_COLOR_YELLOW;
    part3.display = true;
    part3.next = &part4;
    part3.prev = &part2;
    part4.xs = 20;
    part4.ys = 46;
    part4.xe = 22;
    part4.ye = 50;
    part4.color = OLEDC_COLOR_GREEN;
    part4.display = false;
    part4.next = &part5;
    part4.prev = &part3;
    part5.xs = 20;
    part5.ys = 50;
    part5.xe = 22;
    part5.ye = 54;
    part5.color = OLEDC_COLOR_BLUE;
    part5.display = false;
    part5.next = &part6;
    part5.prev = &part4;
    part6.xs = 20;
    part6.ys = 54;
    part6.xe = 22;
    part6.ye = 58;
    part6.color = OLEDC_COLOR_YELLOW;
    part6.display = false;
    part6.next = &part7;
    part6.prev = &part5;
    part7.xs = 20;
    part7.ys = 58;
    part7.xe = 22;
    part7.ye = 62;
    part7.color = OLEDC_COLOR_GREEN;
    part7.display = false;
    part7.next = &part8;
    part7.prev = &part6;
    part8.xs = 20;
    part8.ys = 62;
    part8.xe = 22;
    part8.ye = 66;
    part8.color = OLEDC_COLOR_BLUE;
    part8.display = false;
    part8.next = &part9;
    part8.prev = &part7;
    part9.xs = 20;
    part9.ys = 66;
    part9.xe = 22;
    part9.ye = 70;
    part9.color = OLEDC_COLOR_YELLOW;
    part9.display = false;
    part9.next = &part10;
    part9.prev = &part8;
    part10.xs = 20;
    part10.ys = 70;
    part10.xe = 22;
    part10.ye = 74;
    part10.color = OLEDC_COLOR_GREEN;
    part10.display = false;
    part10.next = NULL;
    part10.prev = &part9;
    last_part = &part3;
}

void __attribute__((__interrupt__))_T1Interrupt(void) {
    if (gameCount < 15) // 4 * 15 = 60
    {
        IFS0bits.T1IF = 0;  // interrupt flag = 0
        gameCount++;
    } else {
        isGameOver = false;
        oledC_setBackground(OLEDC_COLOR_BLACK);
        IEC0bits.T1IE = 0;
    }
}

direction which_direction(unsigned char X, unsigned char Y){
    if (Y < -20 ) {
        return UP;
    } else if (Y > 50 ){
        return DOWN;
    } else if (X > 40 ) { 
        return LEFT;
    } else if ( X < -20 ){
        return RIGHT;
    }
}

void made_charms (struct charm * game_charms){
    srand(time(NULL));
    game_charms[0].color = OLEDC_COLOR_RED;
    game_charms[0].xs = rand() % 192;
    game_charms[0].ys = rand() % 96;
    game_charms[1].color = OLEDC_COLOR_RED;
    game_charms[1].xs = rand() % 192;
    game_charms[1].ys = rand() % 96;
    game_charms[2].color = OLEDC_COLOR_RED;
    game_charms[2].xs = rand() % 192;
    game_charms[2].ys = rand() % 96;
    game_charms[3].color = OLEDC_COLOR_GREEN;
    game_charms[3].xs = rand() % 192;
    game_charms[3].ys = rand() % 96;
    game_charms[4].color = OLEDC_COLOR_GREEN;
    game_charms[4].xs = rand() % 192;
    game_charms[4].ys = rand() % 96;
    game_charms[5].color = OLEDC_COLOR_GREEN;
    game_charms[5].xs = rand() % 192;
    game_charms[5].ys = rand() % 96;
}

void snake_display(struct snake_part* body) {
    while (body != NULL) {
    if (body->display == true && body->ys >= 0) {
        oledC_DrawRectangle(body->xs - x_offset, body->ys, body->xe - x_offset, body->ye, body->color);
    }
    body = body->next;
    }
}

void charms_display(struct charm* charms_) {
    for (int index = 0; index < MAX_CHARMS; index++) {
        if (charms_[index].xs - x_offset >= 0 && charms_[index].xs - x_offset <= 96)
            oledC_DrawCharacter(charms_[index].xs - x_offset, charms_[index].ys, 1, 1, '@', charms_[index].color);
    }
}

bool check_snake_boundary(uint8_t temp_xs, uint8_t temp_ys){
    if (temp_xs < 0 || (temp_xs + 4 ) > 192 || temp_ys == 0 || temp_ys + 4 > 96){
        return false;   // out of boundary
    }
}

void snake_movement(struct snake_head* head, uint8_t temp_xs, uint8_t temp_ys, uint8_t temp_xe, uint8_t temp_ye) {
    
    uint8_t xst, xet, yst, yet;
    
    if (check_snake_boundary( temp_xs, temp_ys)){
        
        struct snake_part* current;

        xst = head->xs;
        yst = head->ys;
        xet = head->xe;
        yet = head->ye;
        head->xs = temp_xs;
        head->ys = temp_ys;
        head->xe = temp_xe;
        head->ye = temp_ye;

        current = head->body;
        
        while (current->next != NULL) {
            current = current->next;
        }
        
        while (current->prev != NULL) {

            current->xs = current->prev->xs;
            current->ys = current->prev->ys;
            current->xe = current->prev->xe;
            current->ye = current->prev->ye;

            current = current->prev;
        }
        
        current->xs = xst;
        current->ys = yst;
        current->xe = xet;
        current->ye = yet;
    }
}

void up_direction(struct snake_head* head, uint8_t* _xs, uint8_t* _ys, uint8_t* _xe, uint8_t * _ye) {
    
    direction d = head->snake_d;
    
    switch(d){
        case UP:
            *_ys = head->ys - 4;
            *_xs = head->xs;
            *_ye = head->ye - 4;
            break;
        case DOWN:
            *_xs = head->xs;
            *_ys = head->ys - 4;
            break;
        case LEFT:
            *_xs = head->xs;
            *_ys = head->ys - 4;
            break;
        case RIGHT:
            *_xs = head->xs + 2;
            *_ys = head->ys - 4;
            break;
    }
    
    *_ye = (*_ys) + 4;
    *_xe = (*_xs) + 2;
    
    head->snake_d = UP;
}

void down_direction(struct snake_head* head, uint8_t* _xs, uint8_t* _ys, uint8_t* _xe, uint8_t * _ye) {
    
    direction d = head->snake_d;
    
    switch(d){
        case UP:
            *_ys = head->ys + 4;
            *_xs = head->xs;
            break;
        case DOWN:
            *_xs = head->xs;
            *_ys = head->ys + 4;
            break;
        case LEFT:
            *_xs = head->xs;
            *_ys = head->ys + 2;
            break;
        case RIGHT:
            *_xs = head->xs + 2;
            *_ys = head->ys + 2;
            break;
    }
    
    *_ye = (*_ys) + 4;
    *_xe = (*_xs) + 2;
    
    head->snake_d = DOWN;

}

void left_direction(struct snake_head* head, uint8_t* _xs, uint8_t* _ys, uint8_t* _xe, uint8_t* _ye) {

    direction d = head->snake_d;
    
    switch(d){
        case UP:
            *_xs = head->xs - 4;
            *_ys = head->ys;
            break;
        case DOWN:
            *_ys = head->ys + 2;
            *_xs = head->xs - 4;
            break;
        case LEFT:
            *_xs = head->xs - 4;
            *_ys = head->ys;
            break;
        case RIGHT:
            *_xs = head->xs - 4;
            *_ys = head->ys;
            break;
    }
    
    *_ye = (*_ys) + 2;
    *_xe = (*_xs) + 4;
    
    head->snake_d = LEFT;
}

void right_direction(struct snake_head* head, uint8_t* _xs, uint8_t* _ys, uint8_t* _xe, uint8_t* _ye) {
    
    direction d = head->snake_d;
    
    switch(d){
        case UP:
            *_xs = head->xs + 3;
            *_ys = head->ys;
            break;
        case DOWN:
            *_ys = head->ys + 2;
            *_xs = head->xs + 2;
            break;
        case LEFT:
            *_xs = head->xs + 4;
            *_ys = head->ys;
            break;
        case RIGHT:
            *_xs = head->xs + 4;
            *_ys = head->ys;
            break;
    }
    
    *_ye = (*_ys) + 2;
    *_xe = (*_xs) + 4;
    
    head->snake_d = RIGHT;
}

void update_snake_direction(int x, int y){
    
        uint8_t temp_xs, temp_ys, temp_xe, temp_ye;
        temp_xs = head.xs;
        temp_xe = head.xe;
        temp_ys = head.ys;
        temp_ye = head.ye;
    
        if (y < -20){
            snake_direction = UP;            
            up_direction(&head, &temp_xs, &temp_ys, &temp_xe, &temp_ye);
            snake_movement(&head, temp_xs, temp_ys, temp_xe, temp_ye);
        } else if (y > 50){
            snake_direction = DOWN;            
            down_direction(&head, &temp_xs, &temp_ys, &temp_xe, &temp_ye);
            snake_movement(&head, temp_xs, temp_ys, temp_xe, temp_ye);
        }else if (x > 40){
            snake_direction = LEFT;            
            left_direction(&head, &temp_xs, &temp_ys, &temp_xe, &temp_ye);
            snake_movement(&head, temp_xs, temp_ys, temp_xe, temp_ye);
        }else if (x < -20){   
            snake_direction = RIGHT;
            right_direction(&head, &temp_xs, &temp_ys, &temp_xe, &temp_ye);
            snake_movement(&head, temp_xs, temp_ys, temp_xe, temp_ye);
        } 
        else { 
            switch(snake_direction){
                case UP:
                    up_direction(&head, &temp_xs, &temp_ys, &temp_xe, &temp_ye);
                    snake_movement(&head, temp_xs, temp_ys, temp_xe, temp_ye);
                    break;
                case DOWN:
                    down_direction(&head, &temp_xs, &temp_ys, &temp_xe, &temp_ye);
                    snake_movement(&head, temp_xs, temp_ys, temp_xe, temp_ye);
                    break;
                case LEFT:
                    left_direction(&head, &temp_xs, &temp_ys, &temp_xe, &temp_ye);
                    snake_movement(&head, temp_xs, temp_ys, temp_xe, temp_ye);
                    break;
                case RIGHT:
                    right_direction(&head, &temp_xs, &temp_ys, &temp_xe, &temp_ye);
                    snake_movement(&head, temp_xs, temp_ys, temp_xe, temp_ye);
                    break;
            }
        }
}

void calculate_result() {
    int i = 0;

    while (i < MAX_CHARMS) {
        if (head.xs >= game_charms[i].xs && head.xs <= game_charms[i].xs + 8 &&
            head.ys >= game_charms[i].ys && head.ys <= game_charms[i].ys + 8) {
            
            if (game_charms[i].color == OLEDC_COLOR_RED) {
                last_part->display = false;
                
                if (last_part->prev != NULL) {
                    last_part = last_part->prev;
                }
                
                game_charms[i].xs = rand() % 192;
                game_charms[i].ys = rand() % 96;
                
            } else if (game_charms[i].color == OLEDC_COLOR_GREEN) {
                if (gameResult == 0) {
                    last_part->display = true;
                } else {
                    if (last_part->next != NULL) {
                        last_part = last_part->next;
                    }
                    
                    last_part->display = true;
                }
                gameResult +=1;
                game_charms[i].xs = rand() % 192;
                game_charms[i].ys = rand() % 96;
            }
        }
        i++;
    }
}

void game_display(){
    oledC_DrawRectangle(head.xs - x_offset, head.ys, head.xe - x_offset, head.ye, head.color);
    snake_display(head.body);
    charms_display(game_charms);
}

void display_end_message(char* s){
    oledC_DrawString(20, 20, 1, 1, "GAME OVER", OLEDC_COLOR_WHITE);
    sprintf(s, "%d", gameResult);
    oledC_DrawString(15, 50, 1, 1, "Your Result:", OLEDC_COLOR_WHITE);
    oledC_DrawString(50, 70, 1, 1, s, OLEDC_COLOR_WHITE);
    for (;;) {
    }
}
/**
 End of File
 */
