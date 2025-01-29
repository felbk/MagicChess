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
#include "hardware/adc.h"

#define UART_ID uart0
#define BAUD_RATE 115200
#define DATA_BITS 8
#define STOP_BITS 1
#define UART_TX_PIN 0
#define UART_RX_PIN 1   
#define digitosmax  5

//PRIMEIRO MOTOR (SUDOESTE)
#define A1 14 //porta conectada ao AI1 do driver 
#define B1 15 // AI2
#define C1 16 //BI1
#define D1 17 //BI2

//SEGUNDO MOTOR (SUDESTE)
#define A2 18 //AI1 (DRIVER)
#define B2 19 //AI2
#define C2 20 //BI1
#define D2 21 //BI2

#define BT1 2 //BOTÃO AZUL (NORTE)
#define BT2 3 // AMARELO (SUL)
#define BT3 4 // VERDE (OESTE)
#define BT4 5 //VERMELHO (LESTE)

#define PWM_A  12 
#define PWM_B  11

#define STDBY  9 //MOTOR 1
#define STDBY2 10 //MOTOR 2
#define DelayMin  100 // tempo minimo de delay em us

#define servopin 22

#define ADC0 26

#define mm_por_step  0.8
#define mm_por_casa 35

#define steps_margin 10

#define maxservo 180 //Posição para atrair a peça 

//variáveis globais de ativação da parada de fim de curso em cada botão 
volatile bool STOP_N = 0;
volatile bool STOP_S = 0;
volatile bool STOP_O = 0;
volatile bool STOP_L = 0;
volatile bool STOP = 0;

//posições da máquina 
int Xm = 1; 
int Ym = 1;

uint8_t tabuleiro[8][8]={
    {'T','C','B','Q','K','B','C','T'},
    {'P','P','P','P','P','P','P','P'},
    {0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 },
    {0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 },
    {0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 },
    {0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 },
    {'P','P','P','P','P','P','P','P'},
    {'T','C','B','Q','K','B','C','T'},
};

//função exponencial 
int expon(int base , int exp){
    int resp = 1;
    for (int i= 0 ; i < exp ; i++){
        resp *= base ;
    }
    return resp;
}

//ativação da variável de parada
void btn_callback(uint gpio, uint32_t events){
    // callbak para fim de curso
    if (events == GPIO_IRQ_EDGE_FALL){
        if (gpio == BT1 || gpio == BT2 || gpio == BT3 || gpio == BT4 ){STOP = 1;}
        if (gpio == BT1){STOP_N = 1;}
        if (gpio == BT2){STOP_S = 1;}
        if (gpio == BT3){STOP_O = 1;}
        if (gpio == BT4){STOP_L = 1;}

    }
}

//inicialização e set de direção dos pinos
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
    gpio_init(STDBY2);
    gpio_init(BT1);
    gpio_init(BT2);
    gpio_init(BT3);
    gpio_init(BT4);
    adc_init();
    adc_gpio_init(ADC0);


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
    gpio_set_dir(STDBY2 , true); // true para saida
    gpio_set_dir(BT1, false); //false para entrada 
    gpio_set_dir(BT2, false);
    gpio_set_dir(BT3, false);
    gpio_set_dir(BT4, false);

//define os botões como pull up interno
    gpio_pull_up(BT1);
    gpio_pull_up(BT2);
    gpio_pull_up(BT3);
    gpio_pull_up(BT4);

//ativa a função de callback para cada botão 
    gpio_set_irq_enabled_with_callback(BT1, GPIO_IRQ_EDGE_FALL , true , &btn_callback);
    gpio_set_irq_enabled(BT2 , GPIO_IRQ_EDGE_FALL , true);
    gpio_set_irq_enabled(BT3 , GPIO_IRQ_EDGE_FALL , true);
    gpio_set_irq_enabled(BT4 , GPIO_IRQ_EDGE_FALL , true);

}


//função para mover o motor de a cordo com o sentido
void passo_motor(int steps , uint8_t Sentido){

//sequências de ativação de cada pino do motor
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
    // int desativada[8][4] = {
    //     {0,0,0,0},
    //     {0,0,0,0},
    //     {0,0,0,0},
    //     {0,0,0,0},
    //     {0,0,0,0},
    //     {0,0,0,0},
    //     {0,0,0,0},
    //     {0,0,0,0}
    // };
//sentido dos motores 1 e 2
    int m1; // true para sentido horario e false para anti horário 
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
        } else if (m1 == 2){
             gpio_put(STDBY, 0);
        }
        if (m2 == 1){
            gpio_put(A2, hor[linha][0]); gpio_put(B2, hor[linha][1]); gpio_put(C2, hor[linha][2]); gpio_put(D2, hor[linha][3]);
            
        }else if (m2 == 0){
            gpio_put(A2, anti[linha][0]); gpio_put(B2, anti[linha][1]); gpio_put(C2, anti[linha][2]); gpio_put(D2, anti[linha][3]);
        } else if(m2 == 2){
              gpio_put(STDBY2, 0);
        }
        
        
        if (m1 == 2 || m2 == 2){ sleep_us(800);} else {sleep_us(800);}
        
    }
    }
    
    }
}

//realiza o passo com o motor
void ctr_motor( uint8_t Sentido , int mm){
    //executa passo
    gpio_put(PWM_A,1);
    gpio_put(PWM_B,1);
    gpio_put(STDBY,1);
    gpio_put(STDBY2,1);
    passo_motor( mm / mm_por_step, Sentido);
    gpio_put(STDBY, 0);
    gpio_put(STDBY2, 0);
}
//move a máquina para o sentido oposto de sua colisão com o botão de fim de curso
void corrigir(){
    printf("\nVoltando para area segura");
    gpio_put(STDBY , 1);
    gpio_put(STDBY2 , 1); 
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
    gpio_put(STDBY2 , 0);
}

//move o motor até que chegue na quina inferior esquerdo (origem)
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

//função para retornar a posição de uma letra maiúscula no alfabeto 
int letra_para_numero(uint8_t letra){
    return (((int)letra)- 64);
}

//função para analisar c 
uint64_t analise(int Xi, int Yi , int Xf , int Yf){
 uint8_t pecaMovida = tabuleiro[Xi][Yi];
 uint8_t destino = tabuleiro[Xf][Yf];
 tabuleiro[Xi][Yi]= 0;
 tabuleiro[Xf][Yf] = pecaMovida;
 if (destino == 0){return 'v';}
 if(destino != 0){return 'c';}else{return 'v';}
 
}
//função para mover uma peça de uma casa a outra
void mover(int Xi , int Yi , int Xf , int Yf){
    
    //servo_set_position(servopin,0);
    
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
        if (!Q && !E && !A && !D && x!= 0 && y!=0){
        if (Xi <=4 ){ctr_motor('N', (mm_por_casa/2));}
        else{ctr_motor('S', (mm_por_casa/2));}
        ctr_motor('L', (mm_por_casa/2));

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
        if (!Q && !E && !A && !D && x != 0 && y!= 0){
        if (Xi <=4 ){ctr_motor('S', (mm_por_casa/2));}
        else{ctr_motor('N', (mm_por_casa/2));}
        ctr_motor('O', (mm_por_casa/2));
        }
                    }
        //Devolve peça 
        sleep_ms(600);
        
        servo_set_position(servopin,0);
        
        sleep_ms(200);
    
    }
}

int coleta_botão(int adcnumber){
    adc_select_input(adcnumber);
    int result = 0 ;
            while (adc_read() < 600){

            }
        for (int i=1 ; i<=100; i ++){
            int leitura = adc_read();
            if (leitura>600){
            result+= leitura * 0.01;}
            else{
                result += leitura / i;
            }
           
            
        }
    int out = 0;
        if (result > 600){      
    if (result <=850){ 
        out=1;
    }
    else if(result <= 980){
        out=2;
    }
    else if(result <= 1110){
      out=3;
    }
    else if(result <=1380){
        out=4;
    }
    else if(result <= 1800){
        out=5;
    }   
    else if(result<=2500){
        out=6;
    }
    else if(result <= 3500){
        out=7;
    }
    else{
        out=8;
    }
    
    }
    
    return out;
}

void Jogando(){
    
    int Xi;
    int Xf;
    int Yi;
    int Yf;
    Xi = coleta_botão(0);
    servo_set_position(servopin,130);
    servo_set_position(servopin,0);

    Yi = coleta_botão(0);
    servo_set_position(servopin,130);
    servo_set_position(servopin,0);

    Xf= coleta_botão(0);
    servo_set_position(servopin,130);
    servo_set_position(servopin,0);

    Yf= coleta_botão(0);
    servo_set_position(servopin,130);
    servo_set_position(servopin,0);
   
    if(analise(Xi,Yi,Xf,Yf)=='c'){
      
            if (Yf <=4 ){mover(Xf,Yf,0,Yf+1);}
            else{mover(Xf,Yf,0,Yf-1);}
        
        
    }
    mover(Xi,Yi,Xf,Yf);
}

void controle_serial(){
      uint8_t ch;
    uint8_t protocol[digitosmax];
    int i=0;
    bool protvalid = 1;
    bool tratar = 0;
    bool aceitar = 0;
    int posbarra =0;

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

int main() {
    stdio_init_all();
    pinos_e_irq();
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

  

    servo_enable(servopin);
    servo_set_position(servopin, 0);
 
    set_origem();

    // mover(7,2 , 7,4);
    
    // mover(6,1 , 8,3);
    
    // mover(2,1 , 3,3);
    
    // mover(1,2 , 1,4);

    // mover(1,1 , 1,3);
    
    // mover(7,1 , 6,3);

    // mover(8,3 , 6,1);

    // mover(5,2 , 5 ,4);

    // mover(6,1 , 1,6);
     
    // mover(8,2 , 8,4);
    
   
    
    
    
   

    while (true) {

        if (!STOP) { //Sem detecção de fim de curso

            Jogando();
          //controle_serial();
            
         
        }

         if (
        STOP ){
        corrigir();
        } 
        
    }
}
