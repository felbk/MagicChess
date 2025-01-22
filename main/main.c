/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "src/pico_servo.h"

#define UART_ID uart0
#define BAUD_RATE 115200
#define DATA_BITS 8
#define STOP_BITS 1
#define UART_TX_PIN 0
#define UART_RX_PIN 1   
#define digitosmax  5

#define A1 14
#define B1 15
#define C1 16
#define D1 17

#define A2 18
#define B2 19
#define C2 20
#define D2 21

#define BT1 2
#define BT2 3
#define BT3 4
#define BT4 5

#define PWM_A  12
#define PWM_B  11

#define STDBY  10
#define DelayMin  100 // tempo minimo de delay em us

#define servopin 22

#define mm_por_step  0.8
#define mm_por_casa 35

#define steps_margin 10

#define maxservo 150

volatile bool STOP_N = 0;
volatile bool STOP_S = 0;
volatile bool STOP_O = 0;
volatile bool STOP_L = 0;
volatile bool STOP = 0;

int Xm = 1;
int Ym = 1;

int expon(int base , int exp){
    int resp = 1;
    for (int i= 0 ; i < exp ; i++){
        resp *= base ;
    }
    return resp;
}

void btn_callback(uint gpio, uint32_t events){
    // callbak para fim de curso
    if (events == GPIO_IRQ_EDGE_FALL){
        STOP = 1;
        if (gpio == BT1){STOP_N = 1;}
        if (gpio == BT2){STOP_S = 1;}
        if (gpio == BT3){STOP_O = 1;}
        if (gpio == BT4){STOP_L = 1;}

    }
}

void pinos_e_irq(void){
    gpio_init(A1);
    gpio_init(B1);
    gpio_init(C1);
    gpio_init(D1);
    gpio_init(A2);
    gpio_init(B2);
    gpio_init(C2);
    gpio_init(D2);
    gpio_init(PWM_A);
    gpio_init(PWM_B);
    gpio_init(STDBY);
    gpio_init(BT1);
    gpio_init(BT2);
    gpio_init(BT3);
    gpio_init(BT4);


    gpio_set_dir(B1 , true); // true para saida
    gpio_set_dir(A1 , true); // true para saida
    gpio_set_dir(C1 , true); // true para saida
    gpio_set_dir(D1 , true); // true para saida
    gpio_set_dir(B2 , true); // true para saida
    gpio_set_dir(A2 , true); // true para saida
    gpio_set_dir(C2 , true); // true para saida
    gpio_set_dir(D2 , true); // true para saida
    gpio_set_dir(PWM_A , true); // true para saida
    gpio_set_dir(PWM_B , true); // true para saida
    gpio_set_dir(STDBY , true); // true para saida
    gpio_set_dir(BT1, false);
    gpio_set_dir(BT2, false);
    gpio_set_dir(BT3, false);
    gpio_set_dir(BT4, false);
    gpio_pull_up(BT1);
    gpio_pull_up(BT2);
    gpio_pull_up(BT3);
    gpio_pull_up(BT4);

    gpio_set_irq_enabled_with_callback(BT1, GPIO_IRQ_EDGE_FALL , true , &btn_callback);
    gpio_set_irq_enabled(BT2 , GPIO_IRQ_EDGE_FALL , true);
    gpio_set_irq_enabled(BT3 , GPIO_IRQ_EDGE_FALL , true);
    gpio_set_irq_enabled(BT4 , GPIO_IRQ_EDGE_FALL , true);

}


void passo_motor(int steps , uint8_t Sentido){
    int hor[8][4] = {
        {1,0,0,0},
        {1,0,1,0},
        {0,0,1,0},
        {0,1,1,0},
        {0,1,0,0},
        {0,1,0,1},
        {0,0,0,1},
        {1,0,0,1}
    };
    int anti[8][4] = {
        {0,0,0,1},
        {0,1,0,1},
        {0,1,0,0},
        {0,1,1,0},
        {0,0,1,0},
        {1,0,1,0},
        {1,0,0,0},
        {1,0,0,1}
    };
    int desativada[8][4] = {
        {0,0,0,0},
        {0,0,0,0},
        {0,0,0,0},
        {0,0,0,0},
        {0,0,0,0},
        {0,0,0,0},
        {0,0,0,0},
        {0,0,0,0}
    };

    int m1; // true para sentido horario
    int m2;
    if (Sentido == 'L'){
        m1 = 1;
        m2 = 1;
    }
      if (Sentido == 'O'){
        m1 = 0;
        m2 = 0;
    }
      if (Sentido == 'N'){
        m1 = 1;
        m2 = 0;
    }
    if (Sentido == 'S'){
        m1 = 0;
        m2 = 1;
    }
     if (Sentido == 'A'){
        m1 = 0;
        m2 = 2;
    }
    if (Sentido == 'E'){
        m1 = 1;
        m2 = 2;
    }
    if (Sentido == 'Q'){
        m1 = 2;
        m2 = 0;
    }
     if (Sentido == 'D'){
        m1 = 2;
        m2 = 1;
    }

    if (m1 ==2 || m2 == 2){steps = steps *2;}
    for ( int i=0 ; i<steps ; i++){
    if (!STOP){
    for (int linha = 0 ; linha<8 ; linha++){
        
        if (m1 == 1){
            gpio_put(A1 , hor[linha][0]); gpio_put(B1 , hor[linha][1]); gpio_put(C1 , hor[linha][2]); gpio_put(D1 , hor[linha][3]);
            
        }else if (m1 == 0){
            gpio_put(A1 , anti[linha][0]); gpio_put(B1 , anti[linha][1]); gpio_put(C1 , anti[linha][2]); gpio_put(D1 , anti[linha][3]);
        } else{
            gpio_put(A1 , desativada[linha][0]); gpio_put(B1 , desativada[linha][1]); gpio_put(C1 , desativada[linha][2]); gpio_put(D1 , desativada[linha][3]);
        }
        if (m2 == 1){
            gpio_put(A2, hor[linha][0]); gpio_put(B2, hor[linha][1]); gpio_put(C2, hor[linha][2]); gpio_put(D2, hor[linha][3]);
            
        }else if (m2 == 0){
            gpio_put(A2, anti[linha][0]); gpio_put(B2, anti[linha][1]); gpio_put(C2, anti[linha][2]); gpio_put(D2, anti[linha][3]);
        } else{
             gpio_put(A2, desativada[linha][0]); gpio_put(B2, desativada[linha][1]); gpio_put(C2, desativada[linha][2]); gpio_put(D2, desativada[linha][3]);
        }
        
        
        if (m1 == 2 || m2 == 2){ sleep_us(800);} else {sleep_us(800);}
        
    }
    }
    
    }
}

void ctr_motor( uint8_t Sentido , int mm){
    //executa passo
    gpio_put(PWM_A,1);
    gpio_put(PWM_B,1);
    gpio_put(STDBY,1);
    passo_motor( mm / mm_por_step, Sentido);
    gpio_put(STDBY, 0);
}
void corrigir(){
    printf("\nVoltando para area segura");
    gpio_put(STDBY , 1); 
    gpio_put(PWM_A , 1); 
    gpio_put(PWM_B , 1); 
    //voltar o motor
    // inverter sentido
    uint8_t inverso ;
    if (STOP_N){ inverso = 'S';}
    if (STOP_S){ inverso = 'N';}
    if (STOP_O){ inverso = 'L';}
    if (STOP_L){ inverso = 'O';}
    printf("\n correção para: ");
    uart_putc(UART_ID, inverso);
    
    STOP =0;
    STOP_N = 0;
    STOP_S = 0;
    STOP_O = 0;
    STOP_L = 0;
    passo_motor(steps_margin,inverso);

    
    gpio_put(STDBY , 0); 
}

void set_origem(){
    while(!STOP_S){
        ctr_motor('S',3000);
    }
    while(STOP){
    corrigir();}
    
    while (!STOP_O){
        ctr_motor('O',3000);
    } 
    while (STOP){
    corrigir();}
        STOP = 0;
        STOP_O = 0;
        STOP_S = 0;
    ctr_motor('N', 10 );
    ctr_motor('L', 40 );
}

int letra_para_numero(uint8_t letra){
    return (((int)letra)- 64);
}

void mover(int Xi , int Yi , int Xf , int Yf){
    servo_set_position(servopin,0);
    int x = (Xi - Xm);
    int y = (Yi - Ym);
    bool E = 0 ;
    bool A = 0;
    bool D = 0 ;
    bool Q = 0;

    if (Xf - Xi == Yf - Yi || -(Xf - Xi) == Yf - Yi ){
        if (Xf - Xi == Yf - Yi){
            if (Xf - Xi >=0) {
                E = 1;
            } else{ A =1 ;}
        }
        else{
            if (Xf - Xi >=0) {
                D = 1;
            } else{ Q =1 ; }
        }
    } 

    for (int i=0 ; i<2; i++){

    if (i == 1){
        x = (Xf - Xi);
        y = (Yf - Yi);
        //coletar peça 
        sleep_ms(200);
        servo_set_position(servopin,maxservo);
        sleep_ms(600);
        //Ir para quina da casa
        if (!Q && !E && !A && !D){
        ctr_motor('N', (mm_por_casa/2)-5);
        ctr_motor('L', (mm_por_casa/2)-5);

        }else{
            if (E){ ctr_motor('E', x * mm_por_casa );

            }
            if (D){ ctr_motor('D', x * mm_por_casa );
                
            }
            if (A){ ctr_motor('A', -x * mm_por_casa );
                
            }
            if (Q){ ctr_motor('Q', -x * mm_por_casa );
                
            }
        }
                    }

    if (i==0 || (!Q && !E && !A && !D)){ 
    
    
    
    if (x >= 0){ //para leste
        ctr_motor('L', x * mm_por_casa );
    } else{ // para oeste
        ctr_motor('O', -x * mm_por_casa );
    }
    if (y >= 0){ //para norte
        ctr_motor('N', y * mm_por_casa );
    } else{ // para sul
        ctr_motor('S', -y * mm_por_casa );
    }
    
    }
    Xm += x;
    Ym += y;
    if (i == 1){
        
        //Ir para meio da casa
        if (!Q && !E && !A && !D){
        ctr_motor('S', (mm_por_casa/2)-5);
        ctr_motor('O', (mm_por_casa/2)-5);}
                    }
        //Devolve peça 
        sleep_ms(600);
        servo_set_position(servopin,0);
        sleep_ms(200);
    
    }
}
int main() {
    stdio_init_all();

    pinos_e_irq();


    

    uart_init(UART_ID, BAUD_RATE);

  
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    
    uint8_t ch;
    uint8_t protocol[digitosmax];
    int i=0;
    bool protvalid = 1;
    bool tratar = 0;
    bool aceitar = 0;
    int posbarra =0;

    
    servo_enable(servopin);

    servo_set_position(servopin, 90);
 
    set_origem();

    mover(4,4 , 3,4);
    sleep_ms(500);
    mover(3,4 , 4,4);
    sleep_ms(500);
    mover(4,4 , 5,4);
    sleep_ms(500);
    mover(5,4 , 4,4);
    sleep_ms(500);
    mover(4,4 , 1,1);
     sleep_ms(500);
    mover(1,1 , 8,8);
    mover(2,3 , 1,5);
   
    
    
    
   

    while (true) {

        if (!STOP) { //Sem detecção de fim de curso

            if (uart_is_readable(UART_ID)){
            //printf("readable \n");
            ch = uart_getc(UART_ID);

            if (i==0 && (ch != 'N' && ch != 'S' && ch != 'L' && ch != 'O' && ch != 'G' && ch != 'Q' && ch != 'A' && ch != 'D' && ch != 'E') ){protvalid = 0 ;} //iniciar sem letra
            if (i >= digitosmax){protvalid = 0 ; } //ultrapassar tamanho
            if (i == digitosmax-1 && ch!='/'){protvalid = 0;} // não finalizar com /

            if (i < digitosmax && !aceitar){
            protocol[i] = ch;
            //printf("Char armazenado \n");
            tratar = 1;
            if (ch == '/'){
                aceitar = 1;
                //printf("Encerramento Detectado \n");
                posbarra = i;
                

            }}

            
            i++;

                
            } else if (tratar){
            printf("tratando... \n");
            if (protvalid && aceitar){
                printf("Protocolo valido\n");
                int j=1;
                int num = 0 ;
                while(protocol[posbarra-j] < 'A'){
                    //uart_putc(UART_ID, protocol[posbarra-j]);
                    num +=  (protocol[posbarra-j] - 48) * expon(10,(j-1)) ;
                    j++;
                   }
                if (protocol[0] == 'G' ){
                    if (num >= 0 && num <=maxservo){
                        servo_set_position(servopin,num);
                    } else{ printf( "\npara controlar o servo, insira um numero entre 0 e 180\n");}
                }
                else if (num == 0){
                    set_origem();
                }else{
                ctr_motor( protocol[0] , (num ));

                }
            }
            
            if (!protvalid){printf("Protocolo invalido\n insira o comando no modelo: X");
            for (int i =0 ; i < digitosmax -2 ; i++){printf("9");}
            printf("/ \n Com X (N, S , O ou L) e 9 (Algarismos) com no máx. o tamanho acima e (/) para finalizar");
            }
            tratar = 0;
            if (aceitar || !protvalid){
                for (int i=0 ; i < digitosmax ; i++) {protocol[i] = 0;}
            protvalid = 1;
            i = 0;
            aceitar = 0 ;} 
            
            
            
            }
         
        }

         if (
        STOP ){
        corrigir();
        } 
        
    }
}
