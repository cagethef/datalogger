#include <SD.h>
#include <SPI.h>

#define PINO_CS_SD 5
#define LED_GRAVACAO 26
#define LED_STATUS 27
#define BOTAO_INICIAR 15
#define BOTAO_PARAR 12

bool gravando = false;
bool problema = false;
bool pausado = false;
File arquivo;
int contador_arquivo = 0;

// Handles para tarefas
TaskHandle_t handleLeds;

void setup() {

  Serial.begin(115200);
  pinMode(LED_GRAVACAO, OUTPUT);
  pinMode(LED_STATUS, OUTPUT);
  pinMode(BOTAO_INICIAR, INPUT_PULLUP);
  pinMode(BOTAO_PARAR, INPUT_PULLUP);

  if (!SD.begin(PINO_CS_SD)) {
    problema = true;
    Serial.println("Falha na inicialização do cartão SD!");
    digitalWrite(LED_STATUS, HIGH);
    return;
  }


  Serial.println("Cartão SD inicializado com sucesso!");
  problema = false;
  digitalWrite(LED_STATUS, LOW);

  // Cria a tarefa para controle de LEDs
  xTaskCreate(gerenciarLeds, "LED Task", 1024, NULL, 1, &handleLeds);
}

void loop() {
  if (problema) return;

  if (digitalRead(BOTAO_INICIAR) == LOW) { // logica p/ iniciar/pausar
    delay(200);
    if (gravando && !pausado) {
      pausado = true;
    } else if (gravando && pausado) {
      pausado = false;
    } else {
      iniciarGravacao();
    }
  }

  if (digitalRead(BOTAO_PARAR) == LOW) {
    delay(200);
    if (gravando) {
      pararGravacao();
    }
  }

  if (gravando && !pausado) {
    gravarDados();
  }
}

// Função para iniciar gravação
void iniciarGravacao() {
  String nomeArquivo = "\datalog_" + String(contador_arquivo++) + ".csv";
  Serial.print("Tentando criar arquivo: ");
  Serial.println(nomeArquivo);
  arquivo = SD.open(nomeArquivo, FILE_WRITE);
  if (arquivo) {
    gravando = true;
    pausado = false;
    Serial.println("Arquivo criado com sucesso!");
  } else {
    problema = true;
    Serial.println("Erro ao criar o arquivo!");
    digitalWrite(LED_STATUS, HIGH); // Problema ao abrir o arquivo
  }
}

// Função para parar gravação
void pararGravacao() {
  if (arquivo) {
    arquivo.close();
  }
  gravando = false;
  pausado = false;
}

// Função para gravar dados
void gravarDados() {
  int adc1 = analogRead(34); // Exemplo de ADCs
  int adc2 = analogRead(35);
  int adc3 = analogRead(32);
  int adc4 = analogRead(33);

  String linha = String(millis()) + "," + String(adc1) + "," + String(adc2) + "," + String(adc3) + "," + String(adc4) + "\n";
  arquivo.print(linha);
  arquivo.flush();
}

// Tarefa para gerenciar LEDs
void gerenciarLeds(void *parameter) {
  unsigned long tempoUltimaMudanca = 0;
  bool estadoLedGravacao = false;

  while (true) {
    if (gravando && !pausado) { // Led vrd piscando ao gravar
      if (millis() - tempoUltimaMudanca > 200) { 
        estadoLedGravacao = !estadoLedGravacao;
        digitalWrite(LED_GRAVACAO, estadoLedGravacao);
        tempoUltimaMudanca = millis();
      }
    } 
    else if (gravando && pausado){
      digitalWrite(LED_GRAVACAO, HIGH);
    }
    else {
      digitalWrite(LED_GRAVACAO, LOW);
    }

    if (problema) {
      digitalWrite(LED_STATUS, HIGH); // Liga o LED indicando um problema.
  } else if (gravando) {
      digitalWrite(LED_STATUS, LOW); 
  } else { // Ocioso led vrm apagado
      digitalWrite(LED_STATUS, LOW); 
  }

    vTaskDelay(100 / portTICK_PERIOD_MS); // Pequeno atraso para evitar loop muito rápido
  }
}