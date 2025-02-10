#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ssd1306.h"
#include "font.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812b.pio.h"

#define LED_PIN 7
#define LED_COUNT 25
#define BOTAO_A 5
#define BOTAO_B 6
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
#define LED_R 13
#define LED_G 11
#define LED_B 12

// variáveis globais
static PIO pio;
static uint sm;
typedef struct{
    uint8_t R;
    uint8_t G;
    uint8_t B;
} led;
volatile led matriz_led[LED_COUNT] = {0};

// assinaturas das funções implementadas 
uint32_t valor_rgb(uint8_t, uint8_t, uint8_t);
void clear_leds(void);
void print_leds(void);
void config(void);

int main(){
    
    // configurações das GPIO
    config();

    // configuração da PIO para a matriz de leds
    pio = pio0; 
    set_sys_clock_khz(128000, false);
    uint offset = pio_add_program(pio, &ws2812b_program);
    sm = pio_claim_unused_sm(pio, true);
    ws2812b_program_init(pio, sm, offset, LED_PIN);
    
    while(true){
        
    }
}

// função que une os dados binários dos LEDs para emitir para a PIO
uint32_t valor_rgb(uint8_t B, uint8_t R, uint8_t G){
  return (G << 24) | (R << 16) | (B << 8);
}

// função que envia dados ao buffer de leds
void set_led(uint8_t indice, uint8_t r, uint8_t g, uint8_t b){
    if(indice < 25){
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

void config(void){
    stdio_init_all();
    
    // Configuração dos LEDs individuais
    gpio_init(LED_R);
    gpio_init(LED_G);
    gpio_init(LED_B);
    gpio_set_dir(LED_R, GPIO_OUT);
    gpio_set_dir(LED_G, GPIO_OUT);
    gpio_set_dir(LED_B, GPIO_OUT);
    
    // Configuração dos botões
    gpio_init(BOTAO_A);
    gpio_init(BOTAO_B);
    gpio_set_dir(BOTAO_A, GPIO_IN);
    gpio_set_dir(BOTAO_B, GPIO_IN);
    gpio_pull_up(BOTAO_A);
    gpio_pull_up(BOTAO_B);
    
    // Inicialização do barramento I2C para o display OLED
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    
    // Inicialização do display SSD1306
    // Estrutura SSD1306
ssd1306_t ssd;

// Inicialização do display SSD1306
ssd1306_init(&ssd, 128, 64, false, endereco, I2C_PORT);
ssd1306_clear(&ssd);
ssd1306_write_string(&ssd, "Sistema Iniciado", 0, 0);

}
