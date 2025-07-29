/**
 * Esse codigo possui funcoes para montar, formatar, listar arquivos e capturar dados do sensor MPU6050
 * e salvá-los no cartão SD.
 */

#include "sd.h"
#include "hw_config.h"
#include "my_debug.h"
#include "mpu6050.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>

#include "pico/stdlib.h"
#include "hardware/rtc.h"

#include "ff.h"
#include "diskio.h"
#include "f_util.h"

extern bool erro_SD; // Declaração da variável global

static spi_t spis[] = {
    {.hw_inst = spi0,
     .miso_gpio = 16,
     .mosi_gpio = 19,
     .sck_gpio = 18,
     .baud_rate = 1000 * 1000}};

static sd_card_t sd_cards[] = {
    {.pcName = "0:",
     .spi = &spis[0],
     .ss_gpio = 17,
     .use_card_detect = false,
     .card_detect_gpio = 22,
     .card_detected_true = -1}};

static char filename[20] = "dadosMPU.csv";

size_t sd_get_num() { return count_of(sd_cards); }

sd_card_t *sd_get_by_num(size_t num)
{
    assert(num < sd_get_num());
    if (num < sd_get_num())
        return &sd_cards[num];
    else
        return NULL;
}

size_t spi_get_num() { return count_of(spis); }

spi_t *spi_get_by_num(size_t num)
{
    assert(num < spi_get_num());
    if (num < spi_get_num())
        return &spis[num];
    else
        return NULL;
}

static sd_card_t *sd_get_by_name(const char *const name)
{
    for (size_t i = 0; i < sd_get_num(); ++i)
        if (0 == strcmp(sd_get_by_num(i)->pcName, name))
            return sd_get_by_num(i);
    DBG_PRINTF("%s: unknown name %s\n", __func__, name);
    return NULL;
}

static FATFS *sd_get_fs_by_name(const char *name)
{
    for (size_t i = 0; i < sd_get_num(); ++i)
        if (0 == strcmp(sd_get_by_num(i)->pcName, name))
            return &sd_get_by_num(i)->fatfs;
    DBG_PRINTF("%s: unknown name %s\n", __func__, name);
    return NULL;
}

void run_setrtc()
{
    const char *dateStr = strtok(NULL, " ");
    if (!dateStr)
    {
        printf("Missing argument\n");
        return;
    }
    int date = atoi(dateStr);

    const char *monthStr = strtok(NULL, " ");
    if (!monthStr)
    {
        printf("Missing argument\n");
        return;
    }
    int month = atoi(monthStr);

    const char *yearStr = strtok(NULL, " ");
    if (!yearStr)
    {
        printf("Missing argument\n");
        return;
    }
    int year = atoi(yearStr) + 2000;

    const char *hourStr = strtok(NULL, " ");
    if (!hourStr)
    {
        printf("Missing argument\n");
        return;
    }
    int hour = atoi(hourStr);

    const char *minStr = strtok(NULL, " ");
    if (!minStr)
    {
        printf("Missing argument\n");
        return;
    }
    int min = atoi(minStr);

    const char *secStr = strtok(NULL, " ");
    if (!secStr)
    {
        printf("Missing argument\n");
        return;
    }
    int sec = atoi(secStr);

    datetime_t t = {
        .year = (int16_t)year,
        .month = (int8_t)month,
        .day = (int8_t)date,
        .dotw = 0,
        .hour = (int8_t)hour,
        .min = (int8_t)min,
        .sec = (int8_t)sec};
    rtc_set_datetime(&t);
}

void run_format()
{
    const char *arg1 = strtok(NULL, " ");
    if (!arg1)
        arg1 = sd_get_by_num(0)->pcName;

    FATFS *p_fs = sd_get_fs_by_name(arg1);
    if (!p_fs)
    {
        printf("Unknown logical drive number: \"%s\"\n", arg1);
        erro_SD = true;
        return;
    }

    FRESULT fr = f_mkfs(arg1, 0, 0, FF_MAX_SS * 2);
    if (FR_OK != fr)
    {
        printf("f_mkfs error: %s (%d)\n", FRESULT_str(fr), fr);
        erro_SD = true;
        return;
    }
    erro_SD = false;
}

void run_mount()
{
    const char *arg1 = strtok(NULL, " ");
    if (!arg1)
        arg1 = sd_get_by_num(0)->pcName;

    FATFS *p_fs = sd_get_fs_by_name(arg1);
    if (!p_fs)
    {
        printf("Unknown logical drive number: \"%s\"\n", arg1);
        erro_SD = true;
        return;
    }

    FRESULT fr = f_mount(p_fs, arg1, 1);
    if (FR_OK != fr)
    {
        printf("f_mount error: %s (%d)\n", FRESULT_str(fr), fr);
        erro_SD = true;
        return;
    }

    sd_card_t *pSD = sd_get_by_name(arg1);
    myASSERT(pSD);
    pSD->mounted = true;
    printf("Processo de montagem do SD ( %s ) concluido\n", pSD->pcName);
    erro_SD = false;
}

void run_unmount()
{
    const char *arg1 = strtok(NULL, " ");
    if (!arg1)
        arg1 = sd_get_by_num(0)->pcName;

    FATFS *p_fs = sd_get_fs_by_name(arg1);
    if (!p_fs)
    {
        printf("Unknown logical drive number: \"%s\"\n", arg1);
        erro_SD = true;
        return;
    }

    FRESULT fr = f_unmount(arg1);
    if (FR_OK != fr)
    {
        printf("f_unmount error: %s (%d)\n", FRESULT_str(fr), fr);
        erro_SD = true;
        return;
    }

    sd_card_t *pSD = sd_get_by_name(arg1);
    myASSERT(pSD);
    pSD->mounted = false;
    pSD->m_Status |= STA_NOINIT;
    printf("SD ( %s ) desmontado\n", pSD->pcName);
    erro_SD = false;
}

void run_getfree()
{
    const char *arg1 = strtok(NULL, " ");
    if (!arg1)
        arg1 = sd_get_by_num(0)->pcName;

    DWORD fre_clust, fre_sect, tot_sect;
    FATFS *p_fs = sd_get_fs_by_name(arg1);
    if (!p_fs)
    {
        printf("Unknown logical drive number: \"%s\"\n", arg1);
        erro_SD = true;
        return;
    }

    FRESULT fr = f_getfree(arg1, &fre_clust, &p_fs);
    if (FR_OK != fr)
    {
        printf("f_getfree error: %s (%d)\n", FRESULT_str(fr), fr);
        erro_SD = true;
        return;
    }

    tot_sect = (p_fs->n_fatent - 2) * p_fs->csize;
    fre_sect = fre_clust * p_fs->csize;

    printf("%10lu KiB total drive space.\n%10lu KiB available.\n", tot_sect / 2, fre_sect / 2);
    erro_SD = false;
}

void run_ls()
{
    const char *arg1 = strtok(NULL, " ");
    if (!arg1)
        arg1 = "";

    char cwdbuf[FF_LFN_BUF] = {0};
    FRESULT fr;
    char const *p_dir;

    if (arg1[0])
        p_dir = arg1;
    else
    {
        fr = f_getcwd(cwdbuf, sizeof cwdbuf);
        if (FR_OK != fr)
        {
            printf("f_getcwd error: %s (%d)\n", FRESULT_str(fr), fr);
            erro_SD = true;
            return;
        }
        p_dir = cwdbuf;
    }

    printf("Directory Listing: %s\n", p_dir);
    DIR dj;
    FILINFO fno;
    memset(&dj, 0, sizeof dj);
    memset(&fno, 0, sizeof fno);

    fr = f_findfirst(&dj, &fno, p_dir, "*");
    if (FR_OK != fr)
    {
        printf("f_findfirst error: %s (%d)\n", FRESULT_str(fr), fr);
        erro_SD = true;
        return;
    }

    while (fr == FR_OK && fno.fname[0])
    {
        const char *pcAttrib;
        if (fno.fattrib & AM_DIR)
            pcAttrib = "directory";
        else if (fno.fattrib & AM_RDO)
            pcAttrib = "read only file";
        else
            pcAttrib = "writable file";

        printf("%s [%s] [size=%llu]\n", fno.fname, pcAttrib, fno.fsize);

        fr = f_findnext(&dj, &fno);
        if (FR_OK != fr)
        {
            printf("f_findnext error: %s (%d)\n", FRESULT_str(fr), fr);
            erro_SD = true;
            f_closedir(&dj);
            return;
        }
    }
    f_closedir(&dj);
    erro_SD = false;
}

void run_cat()
{
    char *arg1 = strtok(NULL, " ");
    if (!arg1)
    {
        printf("Missing argument\n");
        erro_SD = true;
        return;
    }

    FIL fil;
    FRESULT fr = f_open(&fil, arg1, FA_READ);
    if (FR_OK != fr)
    {
        printf("f_open error: %s (%d)\n", FRESULT_str(fr), fr);
        erro_SD = true;
        return;
    }

    char buf[256];
    while (f_gets(buf, sizeof buf, &fil))
        printf("%s", buf);

    fr = f_close(&fil);
    if (FR_OK != fr)
    {
        printf("f_close error: %s (%d)\n", FRESULT_str(fr), fr);
        erro_SD = true;
        return;
    }
    erro_SD = false;
}

void read_file(const char *filename)
{
    FIL file;
    FRESULT res = f_open(&file, filename, FA_READ);
    if (res != FR_OK)
    {
        printf("[ERRO] Nao foi possivel abrir o arquivo para leitura. Verifique se o Cartao esta montado ou se o arquivo existe.\n");
        erro_SD = true;
        return;
    }
    char buffer[128];
    UINT br;
    printf("Conteudo do arquivo %s:\n", filename);
    while (f_read(&file, buffer, sizeof(buffer) - 1, &br) == FR_OK && br > 0)
    {
        buffer[br] = '\0';
        printf("%s", buffer);
    }
    f_close(&file);
    printf("\nLeitura do arquivo %s concluida.\n\n", filename);
    erro_SD = false;
}

void capture_data_and_save()
{
    printf("\nCapturando dados. Aguarde finalizacao...\n");
    FIL file;
    FRESULT res = f_open(&file, filename, FA_WRITE | FA_CREATE_ALWAYS);
    if (res != FR_OK)
    {
        printf("\n[ERRO] Nao foi possivel abrir o arquivo para escrita. Monte o Cartao.\n");
        erro_SD = true;
        return;
    }
    char header[] = "numero_amostra,accel_x,accel_y,accel_z,giro_x,giro_y,giro_z\n";
    UINT bw;
    res = f_write(&file, header, strlen(header), &bw);
    int num_amostras = 0;
    
        
    for (int i = 0; i < 128; i++)
    {
        int16_t acc[3], gyro[3], temp;
        mpu6050_read_raw(acc, gyro, &temp);
        num_amostras++;

        char buffer[100];
        sprintf(buffer, "%d,%d,%d,%d,%d,%d,%d\n", num_amostras, acc[0], acc[1], acc[2], gyro[0], gyro[1], gyro[2]);

        res = f_write(&file, buffer, strlen(buffer), &bw);
        if (res != FR_OK)
        {
            printf("[ERRO] Nao foi possivel escrever no arquivo. Monte o Cartao.\n");
            f_close(&file);
            erro_SD = true;
            return;
        }
        f_sync(&file);
    }
    res = f_close(&file);
    if (res != FR_OK)
    {
        printf("f_close error: %s (%d)\n", FRESULT_str(res), res);
        erro_SD = true;
        return;
    }
    printf("\nDados salvos no arquivo %s.\n\n", filename);
    erro_SD = false;
}

void run_help()
{
    printf("\nComandos disponíveis:\n\n");
    printf("Digite 'a' para montar o cartão SD\n");
    printf("Digite 'b' para desmontar o cartão SD\n");
    printf("Digite 'c' para listar arquivos\n");
    printf("Digite 'd' para mostrar conteúdo do arquivo\n");
    printf("Digite 'e' para obter espaço livre no cartão SD\n");
    printf("Digite 'f' para capturar dados do ADC e salvar no arquivo\n");
    printf("Digite 'g' para formatar o cartão SD\n");
    printf("Digite 'h' para exibir os comandos disponíveis\n");
    printf("\nEscolha o comando:  ");
}

typedef void (*p_fn_t)();

typedef struct
{
    char const *const command;
    p_fn_t const function;
    char const *const help;
} cmd_def_t;

static cmd_def_t cmds[] = {
    {"setrtc", run_setrtc, "setrtc <DD> <MM> <YY> <hh> <mm> <ss>: Define o Relogio de Tempo Real"},
    {"format", run_format, "format [<drive#:>]: Formata o cartao SD"},
    {"mount", run_mount, "mount [<drive#:>]: Monta o cartao SD"},
    {"unmount", run_unmount, "unmount <drive#:>: Desmonta o cartao SD"},
    {"getfree", run_getfree, "getfree [<drive#:>]: Mostra o espaco livre"},
    {"ls", run_ls, "ls: Lista os arquivos"},
    {"cat", run_cat, "cat <filename>: Mostra o conteudo de um arquivo"},
    {"help", run_help, "help: Mostra esta ajuda"}};

void process_stdio(int cRxedChar)
{
    static char cmd[256];
    static size_t ix;

    if (!isprint(cRxedChar) && !isspace(cRxedChar) && '\r' != cRxedChar &&
        '\b' != cRxedChar && cRxedChar != (char)127)
        return;

    printf("%c", cRxedChar);
    stdio_flush();

    if (cRxedChar == '\r')
    {
        printf("%c", '\n');
        stdio_flush();

        if (!strnlen(cmd, sizeof cmd))
        {
            printf("> ");
            stdio_flush();
            return;
        }

        char *cmdn = strtok(cmd, " ");
        if (cmdn)
        {
            size_t i;
            for (i = 0; i < count_of(cmds); ++i)
            {
                if (0 == strcmp(cmds[i].command, cmdn))
                {
                    (*cmds[i].function)();
                    break;
                }
            }
            if (count_of(cmds) == i)
                printf("Command \"%s\" not found\n", cmdn);
        }

        ix = 0;
        memset(cmd, 0, sizeof cmd);
        printf("\n> ");
        stdio_flush();
    }
    else
    {
        if (cRxedChar == '\b' || cRxedChar == (char)127)
        {
            if (ix > 0)
            {
                ix--;
                cmd[ix] = '\0';
            }
        }
        else
        {
            if (ix < sizeof cmd - 1)
            {
                cmd[ix] = cRxedChar;
                ix++;
            }
        }
    }
}
