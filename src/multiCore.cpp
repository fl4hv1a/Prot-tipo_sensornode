#include <Arduino.h>

#define NUM_SENSORES 30
/*const uint32_t INTERVALO_KEEPAWAKE = 5e3; // Tempo Y (ex: 5s)
const uint32_t INTERVALO_DADOS = 15e3;      // Tempo X (ex: 15s)

struct SensorData {
    int id_sensor;
    int leituraBruta;
    unsigned long horario;
};

SensorData leituras[NUM_SENSORES];  // guardará as leituras individuais em cada ciclo de leitura

void keepAwake();   // Mantém o powerbank ativo
void readData(SensorData *leituras);   // Lê dados dos sensores
int writeData(const SensorData *leituras);  // Escreve as leituras no cartão SD (retorna um código de erro)
void showData(const SensorData *leituras);  // Imprime a média aritmética das leituras no display

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
*\/


void setup() {
	xTaskCreate(TaskKeepAwake, "KeepAwake", 2048, NULL, 2, NULL); // Prioridade 2 (Maior)
    xTaskCreate(TaskDados, "Dados", 4096, NULL, 1, NULL);         // Prioridade 1 (Menor)
}

void loop() {
	vTaskDelete(NULL);
}


// Funções:

void keepAwake() {
	;
}

void readData(SensorData *leituras) {
	;
}

int writeData(const SensorData *leituras) {
	return 0;
}

void showData(const SensorData *leituras) {
	;
}
*/