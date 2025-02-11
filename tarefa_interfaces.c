#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ssd1306.h"
#include "font.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "tusb.h"
#include "ws2812b.pio.h"

#define LED_PIN 7
#define LED_COUNT 25
#define BOTAO_A 5
#define BOTAO_B 6
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
#define LED_B 12
#define LED_G 11

// variáveis globais
static PIO pio;
static uint sm;
typedef struct{
    uint8_t R;
    uint8_t G;
    uint8_t B;
} led;
volatile led matriz_led[LED_COUNT] = {0};

/* Esta variável é composta por 4 bits que se portam como flags independentes
    *bit[0] contém o estado do led azul
    *bit[1] quando em 1 o loop manda mensagens para o display e para o monitor serial sobre o led azul
    *bit[2] contém o estado do led verde
    *bit[3] quando em 1 o loop manda mensagens para o display e para o monitor serial sobre o led verde
*/
static volatile uint8_t flag = 0b0000;

// assinaturas das funções implementadas 
uint32_t valor_rgb(uint8_t, uint8_t, uint8_t);
void clear_leds(void);
void print_leds(void);
void set_led(uint8_t, uint8_t, uint8_t, uint8_t);
void config(void);
void gpio_callback(uint, uint32_t);

int main(){

    // configuração de I/O das comunicações seriais
    stdio_init_all();

    // configurações das GPIO, leds e botões
    gpio_init(LED_B);
    gpio_init(LED_G);
    gpio_init(BOTAO_A);
    gpio_init(BOTAO_B);
    gpio_set_dir(LED_B, GPIO_OUT);
    gpio_set_dir(LED_G, GPIO_OUT);
    gpio_set_dir(BOTAO_A, GPIO_IN);
    gpio_set_dir(BOTAO_B, GPIO_IN);
    gpio_pull_up(BOTAO_A);
    gpio_pull_up(BOTAO_B);

    // configurações das interrupções dos botões
    gpio_set_irq_enabled_with_callback(BOTAO_A, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    gpio_set_irq_enabled(BOTAO_B, GPIO_IRQ_EDGE_FALL, true);

    // configuração da PIO para a matriz de leds
    pio = pio0; 
    set_sys_clock_khz(128000, false);
    uint offset = pio_add_program(pio, &ws2812b_program);
    sm = pio_claim_unused_sm(pio, true);
    ws2812b_program_init(pio, sm, offset, LED_PIN);
    // inicialização dos leds apagados
    clear_leds();
    print_leds(); 

    // configurações para o display OLED
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    ssd1306_t ssd;
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    // variável que é escrita pelo monitor serial
    char c;

    // configurações dos leds
    const uint16_t desenho[10] = {
        0x1FBF, // 0
        0x529,  // 1
        0x1DF7, // 2
        0x1DEF, // 3
        0x17E9, // 4
        0x1EEF, // 5
        0x1EFF, // 6 
        0x1D29, // 7
        0x1FFF, // 8
        0x1FEF  // 9
    };
    // leds utilizados para fazer os números
    const uint8_t leds_usados[13] = {1,2,3,8,6,11,12,13,18,16,21,22,23};

    // tela padrão do display OLED
    ssd1306_rect(&ssd, 2, 46, 14, 14, true, false);
    ssd1306_draw_string(&ssd, "LED AZUL  off", 12, 30);
    ssd1306_draw_string(&ssd, "LED VERDE off", 12, 44);
    ssd1306_send_data(&ssd);

    while(true){

        if (flag & (1 << 1)){
            
            if(flag & (1 << 0)){
                printf("O Led azul foi ligado!\n");
                ssd1306_draw_string(&ssd, "LED AZUL  on ", 12, 30);
            } else {
                printf("O Led azul foi desligado!\n");
                ssd1306_draw_string(&ssd, "LED AZUL  off", 12, 30);
            }
            ssd1306_send_data(&ssd);
            flag &= ~(1 << 1);
        }
        
        if (flag & (1 << 3)){
            
            if (flag & (1 << 2)){
                printf("O Led verde foi ligado!\n");
                ssd1306_draw_string(&ssd, "LED VERDE on ", 12, 44);
            } else {
                printf("O Led verde foi desligado!\n");
                ssd1306_draw_string(&ssd, "LED VERDE off", 12, 44);
            }
            ssd1306_send_data(&ssd);
            flag &= ~(1 << 3);
        }

        if (tud_cdc_available()){ 
            c = getchar();
            if ( (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || c == ' '){
                ssd1306_draw_char(&ssd, c, 49, 5);
                ssd1306_send_data(&ssd);

                if (c >= '0' && c <= '9') {
                    clear_leds();  // Apaga antes de desenhar um número
                
                    uint16_t temp = desenho[c - '0'];
                    for (uint8_t i = 0; i < 13; i++) {
                        if (temp & 1) {
                            matriz_led[leds_usados[i]].R = 10;
                        }
                        temp = temp >> 1;
                    }
                    print_leds();  // Atualiza os LEDs com o novo número
                } else {
                    clear_leds();  // Se não for um número, apaga os LEDs
                    print_leds();  // Garante que os LEDs são atualizados corretamente
                }                
            }
        }
    }
}

// função que une os dados binários dos LEDs para emitir para a PIO
uint32_t valor_rgb(uint8_t B, uint8_t R, uint8_t G){
  return (G << 24) | (R << 16) | (B << 8);
}

// função que envia dados ao buffer de leds
void set_led(uint8_t indice, uint8_t r, uint8_t g, uint8_t b){
    if(indice < LED_COUNT){
    matriz_led[indice].R = r;
    matriz_led[indice].G = g;
    matriz_led[indice].B = b;
    }
}

// função que limpa o buffer de leds
void clear_leds(void){
    for(uint8_t i = 0; i < LED_COUNT; i++){
        matriz_led[i].R = 0;
        matriz_led[i].B = 0;
        matriz_led[i].G = 0;
    }
}

// função que manda os dados do buffer para os LEDs
void print_leds(void){
    uint32_t valor;
    for(uint8_t i = 0; i < LED_COUNT; i++){
        valor = valor_rgb(matriz_led[i].B, matriz_led[i].R,matriz_led[i].G);
        pio_sm_put_blocking(pio, sm, valor);
    }
}

/*
Esta função é uma função callback da interrupção dos botões,
ela modifica a flag citada acima para mandar instruções no
loop principal do código
*/
void gpio_callback(uint gpio, uint32_t events){

    //tratamento de debounce dos botões
    static uint32_t tempo_antes = 0;
    uint32_t tempo_agora = to_ms_since_boot(get_absolute_time());
    if (tempo_agora - tempo_antes > 200){
        
        if(gpio == BOTAO_A){
            if (gpio_get(LED_B)){
                gpio_put(LED_B, false);
                flag ^= (1 << 0);
            } else {
                gpio_put(LED_B, true);
                flag ^= (1 << 0);
            }
            flag |= (1 << 1);
        } 

        else if (gpio == BOTAO_B){
            if (gpio_get(LED_G)){
                gpio_put(LED_G, false);
                flag ^= (1 << 2);
            } else {
                gpio_put(LED_G, true);
                flag ^= (1 << 2);
            }
            flag |= (1 << 3);
        }
        tempo_antes = tempo_agora;

    }
    gpio_acknowledge_irq(gpio, events);
}