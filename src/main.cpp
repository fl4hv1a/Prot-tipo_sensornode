#include <Arduino.h>

#define NUM_SENSORES 30
const uint32_t INTERVALO_FUNC1 = 10e3; // 10s
const uint32_t INTERVALO_FUNC2 = 1e3; // 1s

float testeeeeeeeeee = 0.0f;

void func1();
void func2();
void func3();

void TaskA(void *pvParameters) {
	for (;;) {
		func1();

		// Atraso não-bloqueante do SO
		vTaskDelay(INTERVALO_FUNC1 / portTICK_PERIOD_MS);
	}
}

void TaskB(void *pvParameters) {
	for(;;) {
		func2();

		// Atraso não-bloqueante do SO
		vTaskDelay(INTERVALO_FUNC2 / portTICK_PERIOD_MS);
	}
}

void TaskC(void *pvParameters) {
	for(;;) {
		func3();
	}
}

void setup() {
	Serial.begin(115200);

	xTaskCreatePinnedToCore(TaskA, "TaskA", 2048, NULL, 1, NULL, 1);
	xTaskCreatePinnedToCore(TaskB, "TaskB", 4096, NULL, 2, NULL, 1);
	//xTaskCreate(TaskC, "TaskC", 4096, NULL, 1, NULL);
}

void loop() {
	vTaskDelete(NULL);
}


void func1() {
	for (long int i = 0; i < ((long int) 3e6); i++) {
		Serial.printf("Função 1 roda no núcleo: %d; CONTAGEM: %d\n", xPortGetCoreID(), i);
	}
}
void func2() {
	Serial.printf("\n\n\nXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\nFunção 2 roda no núcleo: %d\n\n\n", xPortGetCoreID());
}
void func3() {
	while(1) {
		int a = 2;
		float b = 3.1415926535;

		testeeeeeeeeee = 2 * b * a * a;
	}
}