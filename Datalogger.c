#include <stdio.h>
#include <math.h>

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/rtc.h"
#include "pico/binary_info.h"

#include "lib/mpu6050.h"
#include "lib/sd.h"
#include "lib/ssd1306.h"
#include "lib/font.h"

// Definição dos pinos I2C para o MPU6050
#define I2C_PORT_MPU6050 i2c0
#define I2C_SDA_MPU 0
#define I2C_SCL_MPU 1

// Definição dos pinos I2C para o display OLED
#define I2C_PORT_DISP i2c1
#define I2C_SDA_DISP 14
#define I2C_SCL_DISP 15
#define ENDERECO_DISP 0x3C

#define LED_PIN_GREEN 11
#define LED_PIN_BLUE 12
#define LED_PIN_RED 13

#define BUTTON_A 5
#define BUTTON_B 6

#define BUZZER_PIN 21

#define DEBOUNCE_MS 500

ssd1306_t ssd;

// Ultima vez em que um botao foi pressionado
volatile static uint32_t last_button_press = 0;

int cRxedChar; // Comando que sera aplicado sobre o cartao
bool cartaoMontado = false;
bool botao_A_pressionado = false;
bool botao_B_pressionado = false;
bool erro_SD = false; // Verifica se houve algum erro com o cartao SD

static char filename[20] = "dadosMPU.csv"; // Arquivo onde estao os dados do MPU

// Cabeçalhos de funcoes
static void iniciar();
void gpio_irq_handler(uint gpio, uint32_t events);
static void tocar_leds();
void tocar_buzzer();
void processar_entrada();
void escrever_display();

int main()
{
    stdio_init_all();
    // Iniciar pinos, interrupcoes, I2C...
    iniciar();

    bi_decl(bi_2pins_with_func(I2C_SDA_MPU, I2C_SCL_MPU, GPIO_FUNC_I2C));
    mpu6050_reset();

    run_help();

    while (true)
    {
        cRxedChar = getchar_timeout_us(0);

        // Verifica se algum caractere foi digitado
        if (PICO_ERROR_TIMEOUT != cRxedChar)
        {
            tocar_leds();
            tocar_buzzer();
            escrever_display();

            processar_entrada();
            cRxedChar = getchar_timeout_us(0);
            sleep_ms(1000);

            tocar_leds();
            escrever_display();
        }

        if (botao_A_pressionado)
        {
            if (!cartaoMontado) // Cartao desmontado, monta ele
            {
                cRxedChar = 'a';

                tocar_leds();
                tocar_buzzer();
                escrever_display();

                processar_entrada();
                cRxedChar = getchar_timeout_us(0);
                sleep_ms(1000);

                tocar_leds();
                escrever_display();
            }
            else // Cartao montado, desmonta
            {
                cRxedChar = 'b';

                tocar_leds();
                tocar_buzzer();
                escrever_display();

                processar_entrada();
                cRxedChar = getchar_timeout_us(0);
                sleep_ms(1000);

                tocar_leds();
                escrever_display();
            }
            botao_A_pressionado = false;
        }
        if (botao_B_pressionado) // Comeca a leitura dos dados do MPU
        {
            cRxedChar = 'f';

            tocar_leds();
            tocar_buzzer();
            escrever_display();

            processar_entrada();
            cRxedChar = getchar_timeout_us(0);
            sleep_ms(1000);

            tocar_leds();
            escrever_display();
            botao_B_pressionado = false;
        }
        sleep_ms(100);
    }
}

// Inicia o I2C, display, LEDs, buzzer, botoes e interrupcoes.
static void iniciar()
{
    i2c_init(I2C_PORT_DISP, 400 * 1000);
    gpio_set_function(I2C_SDA_DISP, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_DISP, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_DISP);
    gpio_pull_up(I2C_SCL_DISP);

    ssd1306_init(&ssd, WIDTH, HEIGHT, false, ENDERECO_DISP, I2C_PORT_DISP);
    ssd1306_config(&ssd);
    ssd1306_send_data(&ssd);

    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    i2c_init(I2C_PORT_MPU6050, 400 * 1000);
    gpio_set_function(I2C_SDA_MPU, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_MPU, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_MPU);
    gpio_pull_up(I2C_SCL_MPU);

    gpio_init(LED_PIN_GREEN);
    gpio_set_dir(LED_PIN_GREEN, GPIO_OUT);

    gpio_init(LED_PIN_BLUE);
    gpio_set_dir(LED_PIN_BLUE, GPIO_OUT);

    gpio_init(LED_PIN_RED);
    gpio_set_dir(LED_PIN_RED, GPIO_OUT);

    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);

    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);

    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);

    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
}

// Processa o comando digitado pelo usuario e envia para as funcoes do arquivos 'sd.c'.
void processar_entrada()
{
    switch (cRxedChar)
    {
    case 'a': // Monta o SD
        printf("\nMontando o SD...\n");
        run_mount();
        cartaoMontado = true;
        printf("\nEscolha o comando (h = help):  ");
        break;
    case 'b': // Desmonta o SD
        printf("\nDesmontando o SD. Aguarde...\n");
        run_unmount();
        cartaoMontado = false;
        printf("\nEscolha o comando (h = help):  ");
        break;
    case 'c': // Lista os arquivos presente sno SD
        printf("\nListagem de arquivos no cartão SD.\n");
        run_ls();
        printf("\nListagem concluída.\n");
        printf("\nEscolha o comando (h = help):  ");
        break;
    case 'd': // Le os dados do arquivo
        read_file(filename);
        printf("Escolha o comando (h = help):  ");
        break;
    case 'e': // Lista o espaco disponivel
        printf("\nObtendo espaço livre no SD.\n\n");
        run_getfree();
        printf("\nEspaço livre obtido.\n");
        printf("\nEscolha o comando (h = help):  ");
        break;
    case 'f': // Inicia a captura de dados do MPU
        capture_data_and_save();
        printf("\nEscolha o comando (h = help):  ");
        break;
    case 'g': // Formata o SD
        printf("\nProcesso de formatação do SD iniciado. Aguarde...\n");
        run_format();
        printf("\nFormatação concluída.\n\n");
        printf("\nEscolha o comando (h = help):  ");
        break;
    case 'h': // Mostra a lista de comandos
        run_help();
        break;
    }
}

// Funcao para os LEDs.
static void tocar_leds()
{
    // Erro na leitura do SD, cor roxa.
    if (erro_SD) 
    {
        gpio_put(LED_PIN_GREEN, 0);
        gpio_put(LED_PIN_BLUE, 1);
        gpio_put(LED_PIN_RED, 1);
        sleep_ms(50);
        erro_SD = false;
    }
    // Caractere digitado
    else if (PICO_ERROR_TIMEOUT != cRxedChar)
    {
        switch (cRxedChar)
        {
        case 'a': // Montagem: Amarelo
            gpio_put(LED_PIN_GREEN, 1);
            gpio_put(LED_PIN_BLUE, 0);
            gpio_put(LED_PIN_RED, 1);
            sleep_ms(50);
            break;
        case 'b': // Desmontagem: Amarelo
            gpio_put(LED_PIN_GREEN, 1);
            gpio_put(LED_PIN_BLUE, 0);
            gpio_put(LED_PIN_RED, 1);
            sleep_ms(50);
            break;
        case 'c': // Listagem dos arquivos: pisca em azul
            gpio_put(LED_PIN_GREEN, 0);
            gpio_put(LED_PIN_BLUE, 1);
            gpio_put(LED_PIN_RED, 0);
            sleep_ms(50);
            gpio_put(LED_PIN_BLUE, 0);
            sleep_ms(50);
            gpio_put(LED_PIN_BLUE, 1);
            sleep_ms(50);
            gpio_put(LED_PIN_BLUE, 0);
            break;
        case 'd': // Leitura de arquivo: pisca em azul
            gpio_put(LED_PIN_GREEN, 0);
            gpio_put(LED_PIN_BLUE, 1);
            gpio_put(LED_PIN_RED, 0);
            sleep_ms(50);
            gpio_put(LED_PIN_BLUE, 0);
            sleep_ms(50);
            gpio_put(LED_PIN_BLUE, 1);
            sleep_ms(50);
            gpio_put(LED_PIN_BLUE, 0);
            break;
        case 'f': // Captura de dados: Vermelho
            gpio_put(LED_PIN_GREEN, 0);
            gpio_put(LED_PIN_BLUE, 0);
            gpio_put(LED_PIN_RED, 1);
            sleep_ms(50);
            break;
        }
    }
    // Aguardando comando: Verde
    else
    {
        gpio_put(LED_PIN_GREEN, 1);
        gpio_put(LED_PIN_BLUE, 0);
        gpio_put(LED_PIN_RED, 0);
    }
}

// Funcao para acionar o buzzer
void tocar_buzzer()
{
    // Beep continuo caso haja um erro
    if (erro_SD)
    {
        gpio_put(BUZZER_PIN, 1);
        sleep_ms(1500);
        gpio_put(BUZZER_PIN, 0);
    }
    else if (PICO_ERROR_TIMEOUT != cRxedChar)
    {
        switch (cRxedChar)
        {
        case 'a': // Montagem: um beep
            gpio_put(BUZZER_PIN, 1);
            sleep_ms(500);
            gpio_put(BUZZER_PIN, 0);
            break;
        case 'b': // Desmontagem: um beep
            gpio_put(BUZZER_PIN, 1);
            sleep_ms(500);
            gpio_put(BUZZER_PIN, 0);
            break;
        case 'f': // Captura de dados: dois beeps
            gpio_put(BUZZER_PIN, 1);
            sleep_ms(500);
            gpio_put(BUZZER_PIN, 0);
            sleep_ms(500);
            gpio_put(BUZZER_PIN, 1);
            sleep_ms(500);
            gpio_put(BUZZER_PIN, 0);
            break;
        }
    }
}

// Funcao para escrever no display
void escrever_display()
{
    bool cor = true;
    ssd1306_fill(&ssd, !cor);

    if (erro_SD)
    {
        ssd1306_draw_string(&ssd, "ERRO!", 5, 7);
    }
    else
    {
        switch (cRxedChar)
        {
        case 'a':
            ssd1306_draw_string(&ssd, "Montando", 5, 7);
            ssd1306_draw_string(&ssd, "o SD...", 5, 16);
            break;
        case 'b':
            ssd1306_draw_string(&ssd, "Desmontando...", 5, 7);
            break;
        case 'c':
            ssd1306_draw_string(&ssd, "Listando", 5, 5);
            ssd1306_draw_string(&ssd, "arquivos...", 5, 16);
            break;
        case 'd':
            ssd1306_draw_string(&ssd, "Lendo", 5, 5);
            ssd1306_draw_string(&ssd, "arquivo...", 5, 16);
            break;
        case 'f':
            ssd1306_draw_string(&ssd, "Dados salvos!", 5, 7);
            break;
        case 'g':
            ssd1306_draw_string(&ssd, "Formatando...", 5, 7);
            break;
        default:
            ssd1306_draw_string(&ssd, "Aguardando", 5, 5);
            ssd1306_draw_string(&ssd, "entrada...", 5, 16);
            break;
        }
    }

    // Desenha um cartao SD no display para indicar se ele esta montado ou nao
    for (int i = 0; i < 22; i++)
        ssd1306_hline(&ssd, 44, 84, 32 + i, cartaoMontado);

    ssd1306_hline(&ssd, 61, 67, 54, cartaoMontado);
    ssd1306_hline(&ssd, 61, 66, 55, cartaoMontado);
    ssd1306_hline(&ssd, 61, 65, 56, cartaoMontado);
    ssd1306_hline(&ssd, 61, 64, 57, cartaoMontado);
    ssd1306_hline(&ssd, 44, 57, 54, cartaoMontado);
    ssd1306_hline(&ssd, 44, 56, 55, cartaoMontado);
    ssd1306_hline(&ssd, 44, 55, 56, cartaoMontado);
    ssd1306_hline(&ssd, 44, 55, 57, cartaoMontado);

    ssd1306_rect(&ssd, 27, 2, 125, 36, true, false);
    ssd1306_rect(&ssd, 2, 2, 125, 25, true, false);

    ssd1306_send_data(&ssd);
    sleep_ms(50);
}

// Funcao de interrupcao para debounce dos botoes.
void gpio_irq_handler(uint gpio, uint32_t events)
{
    uint32_t now = to_ms_since_boot(get_absolute_time());

    if ((now - last_button_press) < DEBOUNCE_MS)
        return;

    last_button_press = now;

    if (gpio == BUTTON_A && (events & GPIO_IRQ_EDGE_FALL))
        botao_A_pressionado = true;
    else if (gpio == BUTTON_B && (events & GPIO_IRQ_EDGE_FALL))
        botao_B_pressionado = true;
}