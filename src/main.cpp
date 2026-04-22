#include <Arduino.h>
#include <time.h>
#include <sys/time.h>
#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include <RTClib.h>

#define INTERVALO_KEEPAWAKE 5000U	// 5s
#define INTERVALO_DADOS 15000U	// 15s

// Número de sensores ativos atualmente
#define NUM_SENSORES 30

// Pinos de Seleção dos MUXs
#define PIN_S0 14
#define PIN_S1 27
#define PIN_S2 26
#define PIN_S3 25

// Pinos Enable dos MUXs
#define PIN_MUX1_EN 32
#define PIN_MUX2_EN 33

// Pino de Leitura Analógica dos MUXs
#define PIN_SIG_ADC 34

// Lista com os IDs que possuem sensores conectados.
const int IDS_ATIVOS[NUM_SENSORES] = {
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,	// Mux 1 (0 - 15)
   16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30	// Mux 2 (16 - 31)
};

// Pino CS (Chip Select) do módulo adaptador de cartão SD
#define PIN_SD_CS 5

// RTC externo (com base em um DS3231)
RTC_DS3231 rtc;

struct SensorData {
    int id_sensor;
    int leituraBruta;
    uint32_t horario;
};

SensorData leituras[NUM_SENSORES];  // guardará as leituras individuais em cada ciclo de leitura

/**
 * @brief Mantém o powerbank ativo.
 */
void keepAwake();

/**
 * @brief Faz a leitura de todos os sensores ativos e preenche o array fornecido com elas.
 * 
 * @param leituras Ponteiro para um array com tamanho NUM_SENSORES.
 */
void readData(SensorData *leituras);

/**
 * @brief Grava um array de leituras em um arquivo CSV diário ("YYYYMMDD.csv") no cartão SD.
 * 
 * @param leituras Ponteiro para um array (com tamanho NUM_SENSORES) contendo os dados dos sensores.
 * @return Código de status da execução.
 * @retval 0: Sucesso (dados gravados corretamente).
 * @retval 1: Erro (falha ao abrir o arquivo ou comunicar-se com o cartão SD).
 */
int writeData(const SensorData *leituras);

/**
 * @brief Imprime a média aritmética das leituras no display.
 */
void showData(const SensorData *leituras);

/**
 * @brief Inicializa os pinos dos multiplexadores e desliga-os.
 */
void setupMux();

/**
 * @brief Inicializa a comunicação com o RTC externo.
 */
void setupRTC();

/**
 * @brief Inicializa a comunicação com o cartão SD.
 */
void setupSD();

/**
 * @brief Configura os 4 pinos de seleção para abrir o canal correto (0 a 15)
 */
void setMuxChannel(int channel);

/**
 * @brief Sincroniza o relógio interno do sistema operacional usando um DS3231 (RTC externo)
 */
void syncInternalClock();

/**
 * @brief Retorna a hora atual (unix timestamp) do RTC externo
 */
uint32_t getExtTime();

/**
 * @brief Retorna a hora atual (unix timestamp) do relógio interno do ESP
 */
uint32_t getTime();

void TaskKeepAwake(void *pvParameters) {
	for(;;) {
		keepAwake();

		// Atraso não-bloqueante do SO
        vTaskDelay(INTERVALO_KEEPAWAKE / portTICK_PERIOD_MS); 
	}
}

void TaskDados(void *pvParameters) {
    for (;;) {
        readData(leituras);
        writeData(leituras);
        showData(leituras);

        // Atraso não-bloqueante do SO
        vTaskDelay(INTERVALO_DADOS / portTICK_PERIOD_MS);
    }
}

/*
xTaskCreate(
    FuncaoDaTarefa,   // 1. pvTaskCode
    "NomeDaTarefa",   // 2. pcName
    TamanhoDaPilha,   // 3. usStackDepth (os 2048 / 4096)
    Parametros,       // 4. pvParameters
    Prioridade,       // 5. uxPriority
    &Identificador    // 6. pxCreatedTask
);
*/


void setup() {
	Serial.begin(115200);
	Serial.print("\x1b[20h");	// Imprimir isso conserta o newline no output serial no simulador

    setupRTC();
    setupSD();
    setupMux();

	xTaskCreate(TaskKeepAwake, "KeepAwake", 2048, NULL, 2, NULL); // Prioridade 2 (Maior)
    xTaskCreate(TaskDados, "Dados", 4096, NULL, 1, NULL);         // Prioridade 1 (Menor)
}

void loop() {
	vTaskDelete(NULL);
}

void keepAwake() {
	;
}

void readData(SensorData *leituras) {
    for (int i = 0; i < NUM_SENSORES; i++) {
        int id = IDS_ATIVOS[i];
        int muxIndex = id / 16;   // Resulta em 0 (Mux 1) ou 1 (Mux 2)
        int channel = id % 16;    // Resulta na porta de 0 a 15
        
        // Configura os pinos de seleção do canal
        setMuxChannel(channel);
        
        // Controla os Enables: Liga apenas o Mux necessário e desliga o outro
        if (muxIndex == 0) {
            digitalWrite(PIN_MUX2_EN, HIGH); // Desliga Mux 2
            digitalWrite(PIN_MUX1_EN, LOW);  // Liga Mux 1
        } else {
            digitalWrite(PIN_MUX1_EN, HIGH); // Desliga Mux 1
            digitalWrite(PIN_MUX2_EN, LOW);  // Liga Mux 2
        }
        
        // Aguarda o sinal elétrico estabilizar
        delayMicroseconds(20); 
        
        // Faz a leitura e salva no array
        leituras[i].id_sensor = id;
        leituras[i].leituraBruta = analogRead(PIN_SIG_ADC);
        leituras[i].horario = getTime();
    }
    
    // Desliga ambos os Muxes ao terminar o ciclo de leitura
    digitalWrite(PIN_MUX1_EN, HIGH);
    digitalWrite(PIN_MUX2_EN, HIGH);
}

int writeData(const SensorData *leituras) {
    // Usamos o horário da primeira leitura do array para saber em qual dia estamos
    time_t rawtime = (time_t) leituras[0].horario;
    struct tm * timeinfo = localtime(&rawtime);

    char filename[16]; 
    
    // Formata o nome do arquivo corretamente: "/YYYYMMDD.csv\0"
    sprintf(filename, "/%04d%02d%02d.csv",
        timeinfo->tm_year + 1900,   // tm_year conta anos desde 1900
        timeinfo->tm_mon + 1,       // tm_mon começa do 0
        timeinfo->tm_mday
    );
    
    // Verifica se o arquivo ja existia antes de abrir
    bool arquivoExistia = SD.exists(filename);
    
    // FILE_APPEND vai adicionar dados ao final (se o arquivo existir)
    // ou criar um arquivo novo (se for a primeira leitura do dia)
    File dataFile = SD.open(filename, FILE_APPEND);

    // Se o cartão SD foi ejetado ou corrompeu
    if (!dataFile) {
        Serial.println("ERRO: Não foi possível abrir o arquivo no cartão SD!");
        return 1; // Retorna código de erro
    }

    // Se o arquivo acabou de ser criado, escreve o cabeçalho na primeira linha
    if (!arquivoExistia) {
        dataFile.println("Timestamp,Sensor_ID,Valor");
    }

    // Escreve os dados no formato: Timestamp,ID,Valor
    for (int i = 0; i < NUM_SENSORES; i++) {
        dataFile.print(leituras[i].horario);
        dataFile.print(",");
        dataFile.print(leituras[i].id_sensor);
        dataFile.print(",");
        dataFile.println(leituras[i].leituraBruta); // o println adiciona a quebra de linha (\n)
    }

    // Fecha o arquivo e grava os dados físicamente no cartão SD
    dataFile.close();

    return 0;
}

void showData(const SensorData *leituras) {
	;
}

void setupMux() {
    pinMode(PIN_S0, OUTPUT);
    pinMode(PIN_S1, OUTPUT);
    pinMode(PIN_S2, OUTPUT);
    pinMode(PIN_S3, OUTPUT);
    
    pinMode(PIN_MUX1_EN, OUTPUT);
    pinMode(PIN_MUX2_EN, OUTPUT);
    
    // Desliga ambos os Muxes inicialmente (Assumindo Enable Active-LOW)
    digitalWrite(PIN_MUX1_EN, HIGH);
    digitalWrite(PIN_MUX2_EN, HIGH);
}

void setupRTC() {
    // Inicializa o barramento I2C (SDA e SCL) necessário para o RTC externo
    Wire.begin();

    if (!rtc.begin()) { // Verifica se a comunicação foi inicializada com sucesso

        Serial.println("ERRO: RTC não encontrado no barramento I2C!");
        Serial.println("Leituras incluirão o horário incorreto (1970)!");

    } else if (rtc.lostPower()) { // Verifica se a bateria do RTC acabou ou foi removida (OSF Flag)

        Serial.println("ERRO: O RTC perdeu energia!");
        Serial.println("Leituras incluirão o horário incorreto!");

    } else {
        syncInternalClock();
    }
}

void setupSD() {
    if (!SD.begin(PIN_SD_CS)) {
        Serial.println("ERRO: Falha ao inicializar o Cartão SD!");
        Serial.println("Verifique as conexões SPI (MOSI, MISO, SCK, CS)");
    }
}

void setMuxChannel(int channel) {
	// Usamos bitRead para ler bit a bit o número do canal (0 a 15)
    // e enviar HIGH ou LOW para os pinos S0-S3
    digitalWrite(PIN_S0, bitRead(channel, 0));
    digitalWrite(PIN_S1, bitRead(channel, 1));
    digitalWrite(PIN_S2, bitRead(channel, 2));
    digitalWrite(PIN_S3, bitRead(channel, 3));
}

void syncInternalClock() {
    uint32_t rtcTimestamp = getExtTime();

    // Estrutura que o FreeRTOS (SO do ESP) utiliza
    struct timeval tv;
    tv.tv_sec = rtcTimestamp; // Segundos passados desde 1970
    tv.tv_usec = 0;           // Microssegundos

    // Injeta a hora no relógio interno
    settimeofday(&tv, NULL);
}

uint32_t getExtTime() {
    return rtc.now().unixtime();
}

uint32_t getTime() {
    time_t timestamp = time(NULL);
    return (uint32_t) timestamp;
}