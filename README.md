# Datalogger com a Raspberry Pi Pico W
**EMBARCATECH - Fase 2**

**Sistema de armazenamento de dados utilizando a BitDogLab**

## Desenvolvedor
- **Carlos Henrique Silva Lopes**

---

### 🎯 Objetivo Geral

Utilizar o sensor MPU6050 para capturar dados de acelerômetro e de giroscópio e armazená-los em um cartão mini SD.

---

### 📄 Descrição

Este é um projeto que utiliza o sensor MPU6050, que captura dados de acelerômetro e giroscópio em 3 eixos. Esses dados serão capturados e armazenados em um cartão mini SD
conectado à placa BitDogLab.
O sistema utiliza LEDs, buzzer, display e saída pelo monitor serial para feedback ao usuário. Também faz o uso de botões para realizar ações sobre o cartão.
Os dados capturados vão para um arquivo .csv, e então, é utilizado um código em Python para organizar os dados do acelerômetro e giroscópio em gráficos de linha separados.

---

### ⚙️ Funcionalidades

* **LEDs RGB:** Acende em verde quando estiver esperando algum comando. Acende em amarelo quando está montando/desmontando o botão. Vermelho quando está realizando a leitura do sensor. Pisca em azul quando está listando os arquivos no cartão ou lendo dados de um dos arquivos. Pisca em roxo caso algum erro tenha ocorrido.
* **Display SSD1306:** Informa quando o sistema está aguardando alguma leitura, realizando alguma operação ou caso algum erro tenha ocorrido. Além disso, um cartão SD é desenhado sempre que estiver montado.
* **Buzzer:** Sons para quando o sistema apresenta um erro, quando está desmontando ou montando o cartão, e quando está realizando a captura de
dados.
* **Botão A:** Botão bom debounce, faz a montagem/desmontagem do cartão.
* **Botão B:** Botão bom debounce, inicia a captura dos dados do sensor.

---

### 📌 Mapeamento de Pinos

| Função             | GPIO |
| ------------------ | ---- |
| Botão A            | 5    |
| Botão B            | 6    |
| LED Verde          | 11   |
| LED Azul           | 12   |
| LED Vermelho       | 13   |
| Buzzer             | 21   |


---

### Principais Arquivos
- **`Datalogger.c`**: Contém a lógica principal do programa. Funções para os LEDs, buzzer, interrupção para os botões, escita no display.
- **`lib/`**: Contém arquivos separados, para o display, MPU6050 e o cartão SD.
- **`lib/sd.c`**:  Contém a lógica para ler os arquivos do SD, fazer sua montagem, desmontagem, etc.
- **`lib/mpu6050.c`**:  Contém funções para iniciar o sensor, e capturar dados de acelerômetro e giroscópio.
- **`lib/ssd1306.c`**: Contém as funções para desenhar e escrever no display ssd1306.
- **`ArquivosDados/`**: Arquivo dos dados que foram capturados do sensor, em .csv, e um script em python para criar gráficos.
- **`README.md`**: Documentação detalhada do projeto.
