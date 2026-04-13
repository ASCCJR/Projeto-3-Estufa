/**
 * @file configura_geral.h
 * @brief Configuracoes globais de hardware, rede, topicos e estados do firmware.
 */

#ifndef CONFIGURA_GERAL_H
#define CONFIGURA_GERAL_H

#include "pico/stdlib.h"
#include "secrets.h"

// --- Override local por maquina (arquivo ignorado no git) ---
#if defined(__has_include)
#if __has_include("configura_local.h")
#include "configura_local.h"
#endif
#endif

// --- Hardware: pinos e perifericos ---
#define LED_R 13
#define LED_G 11
#define LED_B 12
#define BUZZER_PIN 21
#define SERVO_PIN 2
#define MATRIZ_PIN 7
#define BOTAO_B_PIN 6

#define SDA_PIN 14
#define SCL_PIN 15
#define I2C0_SDA_PIN 0
#define I2C0_SCL_PIN 1

#define PWM_MAX_DUTY 0xFFFF

// --- Rede e MQTT ---

#ifndef DEVICE_ID
#define DEVICE_ID "bitdoglab_02"
#endif

#ifndef MQTT_BROKER_IP
#define MQTT_BROKER_IP "127.0.0.1"
#endif

#ifndef MQTT_BROKER_PORT
#define MQTT_BROKER_PORT 1884
#endif

// --- Sensores e tempos ---
#define LUZ_MAXIMA_ESTUFA 2000.0
#define TEMPO_MSG_BEM_VINDO_US 2500000
#define TEMPO_MSG_IRRIGACAO_FIM_US 1500000
#define TEMPO_IRRIGACAO_S 10 // duracao da irrigacao em segundos

// --- Topicos MQTT ---
#define TOPICO_BASE_COMANDO_ESTADO "comando/estado"
#define TOPICO_HISTORICO "historico"
#define TOPICO_HEARTBEAT "heartbeat"

// --- Comandos FIFO inter-core ---
#define FIFO_CMD_WIFI_CONECTADO 0xFFFE
#define FIFO_CMD_PUBLICAR_MQTT 0xADD0
#define FIFO_CMD_MUDAR_ESTADO 0xE5A0
#define FIFO_CMD_MQTT_CONECTADO 0xBEEF
#define FIFO_CMD_PUB_SENSOR_TEMP 0xADD2
#define FIFO_CMD_PUB_SENSOR_UMID 0xADD3
#define FIFO_CMD_PUB_SENSOR_LUZ 0xADD4

// --- Estados e tipos ---
enum ModoOperacao {
    MODO_ESTUFA_OK,
    MODO_ESTUFA_ALERTA_LUZ,
    MODO_ESTUFA_PROTEGENDO,
    MODO_ESTUFA_PROTEGIDO,
    MODO_ESTUFA_IRRIGACAO,
    MODO_MSG_IRRIGACAO_FIM
};

enum WifiStatus {
    WIFI_STATUS_FAIL, WIFI_STATUS_SUCCESS
};

enum MQTT_MSG_TYPE {
    MSG_ALARM_LUZ_ON,
    MSG_ALARM_LUZ_OFF,
    MSG_LOG_HEARTBEAT
};

#endif // CONFIGURA_GERAL_H