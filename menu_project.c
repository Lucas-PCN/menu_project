#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "stdlib.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "ssd1306.h"

// Definições de pinos
#define I2C_SDA_PIN 14
#define I2C_SCL_PIN 15
#define LED_R_PIN 13
#define LED_B_PIN 12
#define LED_G_PIN 11
#define BUZZER_PIN 21
#define JOY_X_PIN 27
#define JOY_Y_PIN 26
#define JOY_BTN_PIN 22
#define ADC_CHANNEL_0 0 // Canal ADC para o eixo X do joystick
#define ADC_CHANNEL_1 1 // Canal ADC para o eixo Y do joystick

// Variáveis globais
bool last_btn_state = true;
bool in_menu = true;
uint32_t last_btn_press_time = 0;

// Variáveis do menu
const char *menu_items[] = {"Joystick LED", "Tocar Buzzer", "Ligar LED RGB"};
int selected_option = 0;

// Buffer do display e área de renderização
uint8_t ssd_buffer[ssd1306_buffer_length];
struct render_area frame_area;

// Notas musicais para a música tema de Star Wars
const uint star_wars_notes[] = {
    330, 330, 330, 262, 392, 523, 330, 262,
    392, 523, 330, 659, 659, 659, 698, 523,
    415, 349, 330, 262, 392, 523, 330, 262,
    392, 523, 330, 659, 659, 659, 698, 523,
    415, 349, 330, 523, 494, 440, 392, 330,
    659, 784, 659, 523, 494, 440, 392, 330,
    659, 659, 330, 784, 880, 698, 784, 659,
    523, 494, 440, 392, 659, 784, 659, 523,
    494, 440, 392, 330, 659, 523, 659, 262,
    330, 294, 247, 262, 220, 262, 330, 262,
    330, 294, 247, 262, 330, 392, 523, 440,
    349, 330, 659, 784, 659, 523, 494, 440,
    392, 659, 784, 659, 523, 494, 440, 392
};

// Duração das notas em milissegundos
const uint note_duration[] = {
    500, 500, 500, 350, 150, 300, 500, 350,
    150, 300, 500, 500, 500, 500, 350, 150,
    300, 500, 500, 350, 150, 300, 500, 350,
    150, 300, 650, 500, 150, 300, 500, 350,
    150, 300, 500, 150, 300, 500, 350, 150,
    300, 650, 500, 350, 150, 300, 500, 350,
    150, 300, 500, 500, 500, 500, 350, 150,
    300, 500, 500, 350, 150, 300, 500, 350,
    150, 300, 500, 350, 150, 300, 500, 500,
    350, 150, 300, 500, 500, 350, 150, 300,
};



// Funções

// Inicializa o PWM no pino do buzzer
void pwm_init_buzzer(uint pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(pin);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 4.0f); // Ajusta divisor de clock
    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(pin, 0); // Desliga o PWM inicialmente
}

// Toca uma nota com a frequência e duração especificadas
void play_tone(uint pin, uint frequency, uint duration_ms) {
    uint slice_num = pwm_gpio_to_slice_num(pin);
    uint32_t clock_freq = clock_get_hz(clk_sys);
    uint32_t top = clock_freq / frequency - 1;

    pwm_set_wrap(slice_num, top);
    pwm_set_gpio_level(pin, top / 2); // 50% de duty cycle

    sleep_ms(duration_ms);

    pwm_set_gpio_level(pin, 0); // Desliga o som após a duração
    sleep_ms(50); // Pausa entre notas
}


// Função para configurar os LEDs
void set_leds(bool red, bool green, bool blue){
    gpio_put(LED_R_PIN, red);
    gpio_put(LED_G_PIN, green);
    gpio_put(LED_B_PIN, blue);
}

// Função para inicializar o display OLED
void init_oled() {
    i2c_init(i2c1, 100 * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    
    ssd1306_init();
    
    frame_area.start_column = 0;
    frame_area.end_column = ssd1306_width - 1;
    frame_area.start_page = 0;
    frame_area.end_page = ssd1306_n_pages - 1;
    
    calculate_render_area_buffer_length(&frame_area);
    
    memset(ssd_buffer, 0, ssd1306_buffer_length);
    render_on_display(ssd_buffer, &frame_area);
    sleep_ms(100);
}

// Função para exibir o menu
void display_menu() {
    memset(ssd_buffer, 0, ssd1306_buffer_length);
    for (int i = 0; i < 3; i++) {
        if (i == selected_option) {
            ssd1306_draw_string(ssd_buffer, 5, i * 10, "> ");
            ssd1306_draw_string(ssd_buffer, 15, i * 10, (char*)menu_items[i]);
        } else {
            ssd1306_draw_string(ssd_buffer, 5, i * 10, (char*)menu_items[i]);
        }
    }
    render_on_display(ssd_buffer, &frame_area);
}

// Função para debounce do botão
bool debounce_button(uint gpio) {
    static uint32_t last_ms = 0;
    const uint32_t debounce_delay = 50; // Tempo de debounce em milissegundos

    if (gpio_get(gpio) == 0) { // Botão pressionado
        if (time_us_32() - last_ms > debounce_delay * 1000) {
            last_ms = time_us_32() / 1000;
            return true; // Retorna verdadeiro apenas uma vez após o debounce
        }
    } else {
        last_ms = 0; // Reseta o tempo se o botão for liberado
    }

    return false;
}

// Funções para executar as opções do menu

// Função para controlar os LEDs com o joystick
bool joystick_led_running = false; // Flag para controlar se estamos em joystick_led

void joystick_led() {
    sleep_ms(200);
    joystick_led_running = true;

    while (joystick_led_running) {
        sleep_ms(200);
        // Leitura dos valores do joystick
        adc_select_input(1);
        uint adc_y_raw = adc_read();
        adc_select_input(0);
        uint adc_x_raw = adc_read();

        // Controle dos LEDs com base nos valores do joystick
        if (adc_x_raw > 3500) {
            set_leds(1, 0, 0);  // Vermelho
        } else if (adc_x_raw < 500) {
            set_leds(0, 0, 1);  // Azul
        } else if (adc_y_raw > 3500) {
            set_leds(0, 1, 0);  // Verde
        } else if (adc_y_raw < 500) {
            set_leds(1, 1, 0);  // Amarelo
        } else {
            set_leds(0, 0, 0);  // Apaga todos
        }

        // Verifica o estado do botão *após* a interação com o joystick
        if (debounce_button(JOY_BTN_PIN)) { // Usa a função de debounce
            joystick_led_running = false; // Sai da função e retorna ao menu
            in_menu = true; // Garante que in_menu seja verdadeiro
            display_menu(); // Exibe o menu novamente
            break;
        }

        sleep_ms(100); // Delay para evitar leituras rápidas demais
    }
}

bool tocar_buzzer_running = false; // Flag para controlar se estamos em tocar_buzzer

// Função principal para tocar a música
void play_star_wars(uint pin) {
  sleep_ms(100);
    for (int i = 0; i < sizeof(star_wars_notes) / sizeof(star_wars_notes[0]); i++) {
        sleep_ms(100);
        // Se o botão for pressionado, interrompe a música
        if (debounce_button(JOY_BTN_PIN)) {
            tocar_buzzer_running = false;
            in_menu = true;
            display_menu();
            return; // Sai imediatamente da função
        }

        if (star_wars_notes[i] == 0) {
            sleep_ms(note_duration[i]);
        } else {
            play_tone(pin, star_wars_notes[i], note_duration[i]);
        }
    }
}

void tocar_buzzer() {
    sleep_ms(100);
    tocar_buzzer_running = true;

    play_star_wars(BUZZER_PIN); // Toca a música, mas agora pode ser interrompida

    // Depois que a música termina ou foi interrompida, volta ao menu
    in_menu = true;
    display_menu();
}


bool led_pwm_running = false; // Flag para controlar se estamos no modo LED PWM

void ligar_led_pwm() {
    sleep_ms(100);
    led_pwm_running = true;

    // Configuração do PWM para o LED
    gpio_set_function(LED_B_PIN, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(LED_B_PIN);
    pwm_set_clkdiv(slice, 16.0f); // Reduz frequência para melhor visibilidade
    pwm_set_wrap(slice, 1000);    // Define o range do PWM
    pwm_set_gpio_level(LED_B_PIN, 0);
    pwm_set_enabled(slice, true);

    uint16_t led_level = 0;
    bool aumentando = true;

    while (led_pwm_running) {
        pwm_set_gpio_level(LED_B_PIN, led_level);
        sleep_ms(20); // Ajuste para suavizar o efeito

        if (aumentando) {
            led_level += 50;
            if (led_level >= 1000) aumentando = false;
        } else {
            led_level -= 50;
            if (led_level <= 0) aumentando = true;
        }

        // Se o botão for pressionado, sair do loop
        
        if (gpio_get(JOY_BTN_PIN) == 0) { // Verifica se o botão foi pressionado
            sleep_ms(200); // Debounce simples
            if (gpio_get(JOY_BTN_PIN) == 0) { // Confirma se ainda está pressionado
                led_pwm_running = false;
                pwm_set_gpio_level(LED_B_PIN, 0); // Apaga o LED antes de sair
                pwm_set_enabled(slice, false);   // Desativa o PWM do LED Azul
                gpio_set_function(LED_B_PIN, GPIO_FUNC_SIO); // Reseta para controle normal
                gpio_set_dir(LED_B_PIN, GPIO_OUT);
                gpio_put(LED_B_PIN, 0); // Garante que o LED azul fique apagado
                in_menu = true;
                display_menu();
                return;
            }
        }
    }
}

// Função para tratar a seleção do menu
void select_option() {
    if (in_menu) {
        switch (selected_option) {
            case 0: joystick_led(); break;
            case 1: tocar_buzzer(); break;
            case 2: ligar_led_pwm(); break;
        }
    } else {
        in_menu = true; // Retorna ao menu
        display_menu();
    }
}



int main() {
    stdio_init_all();

    adc_init();
    adc_gpio_init(JOY_Y_PIN);
    adc_gpio_init(JOY_X_PIN);

    // Configuração do botão do joystick
    gpio_init(JOY_BTN_PIN);
    gpio_set_dir(JOY_BTN_PIN, GPIO_IN);
    gpio_pull_up(JOY_BTN_PIN);

    // Configuração dos LEDs e buzzer
    gpio_init(LED_R_PIN);
    gpio_set_dir(LED_R_PIN, GPIO_OUT);
    gpio_init(LED_G_PIN);
    gpio_set_dir(LED_G_PIN, GPIO_OUT);
    gpio_init(LED_B_PIN);
    gpio_set_dir(LED_B_PIN, GPIO_OUT);

    pwm_init_buzzer(BUZZER_PIN);

    // Inicializa o display e exibe o menu
    init_oled();
    display_menu();
    sleep_ms(100);

    while (1) {
        if (in_menu) {
            int y_value = adc_read();

            // Zona morta para evitar variações pequenas
            if (y_value > 3000) {
                selected_option = (selected_option + 1) % 3;
                display_menu();
                sleep_ms(300); // Delay para evitar mudança rápida
            } else if (y_value < 1000) {
                selected_option = (selected_option - 1 + 3) % 3;
                display_menu();
                sleep_ms(300); // Delay para evitar mudança rápida
            }
        }

        if (debounce_button(JOY_BTN_PIN)) { // Usa a função de debounce no menu também
            select_option();
        }

        sleep_ms(100); // Delay geral
    }

    return 0;
}