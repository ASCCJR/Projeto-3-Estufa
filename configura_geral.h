/**
 * @file configura_geral.h
 * @brief Definições de configuração para o projeto Estufa Inteligente.
 */

#ifndef CONFIGURA_GERAL_H
#define CONFIGURA_GERAL_H

#include "pico/stdlib.h"
#include "secrets.h"

// --- Definições de Hardware e Pinos ---
#define LED_R 13
#define LED_G 11
#define LED_B 12
#define BUZZER_PIN 21
#define SERVO_PIN 2
#define MATRIZ_PIN 7
#define BOTAO_B_PIN 6

// Display OLED e Sensores I2C
#define SDA_PIN 14 // I2C1 (Display)
#define SCL_PIN 15 // I2C1 (Display)
#define I2C0_SDA_PIN 0 // I2C0 (Sensores AHT10, BH1750)
#define I2C0_SCL_PIN 1 // I2C0 (Sensores AHT10, BH1750)

#define PWM_MAX_DUTY 0xFFFF

// --- Configurações de Rede e MQTT ---
#define DEVICE_ID "bitdoglab_02"
#define MQTT_BROKER_IP "192.168.0.18"
#define MQTT_BROKER_PORT 1883

// --- Limiares de Sensores ---
#define LUZ_MAXIMA_ESTUFA 2000.0

// Definições de Timers (em microssegundos)
#define TEMPO_MSG_BEM_VINDO_US 2500000
#define TEMPO_MSG_IRRIGACAO_FIM_US 1500000

// --- Tópicos MQTT ---
#define TOPICO_BASE_COMANDO_ESTADO "comando/estado"
#define TOPICO_HISTORICO "historico"
#define TOPICO_HEARTBEAT "heartbeat"

// --- Comandos FIFO ---
#define FIFO_CMD_WIFI_CONECTADO 0xFFFE
#define FIFO_CMD_PUBLICAR_MQTT 0xADD0
#define FIFO_CMD_MUDAR_ESTADO 0xE5A0
#define FIFO_CMD_MQTT_CONECTADO 0xBEEF
#define FIFO_CMD_PUB_SENSOR_TEMP 0xADD2
#define FIFO_CMD_PUB_SENSOR_UMID 0xADD3
#define FIFO_CMD_PUB_SENSOR_LUZ 0xADD4

// --- Enumerações de Estado e Tipos ---
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