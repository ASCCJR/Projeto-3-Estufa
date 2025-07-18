/**
 * @file bh1750.c
 * @brief Implementação do driver para o sensor de luminosidade I2C BH1750.
 */

#include "pico/stdlib.h" // Para sleep_ms
#include "bh1750.h"      // Para o próprio cabeçalho do driver


// --- Definições Internas de Registradores e Comandos do Sensor ---
const uint8_t BH1750_ADDR = 0x23;          // Endereço I2C padrão do sensor
const uint8_t BH1750_CMD_POWER_ON = 0x01;  // Comando para ligar o sensor
const uint8_t BH1750_CMD_RESET = 0x07;     // Comando para resetar o sensor
const uint8_t BH1750_CMD_HIRES1 = 0x20;    // Comando para modo de alta resolução (1 lux, tempo de conversão ~120ms)


// --- Implementação das Funções Públicas ---

/**
 * @brief Inicializa o sensor BH1750.
 * Envia os comandos iniciais para ligar e resetar o sensor.
 * @param i2c A instância do I2C a ser usada.
 */
void bh1750_init(i2c_inst_t *i2c) {
    // Envia a sequência de inicialização para o sensor: Power On e Reset.
    // Embora essas escritas não verifiquem o retorno, a falha real seria detectada
    // na primeira tentativa de leitura em bh1750_read_lux().
    i2c_write_blocking(i2c, BH1750_ADDR, &BH1750_CMD_POWER_ON, 1, false);
    sleep_ms(10); // Pequeno atraso após ligar
    i2c_write_blocking(i2c, BH1750_ADDR, &BH1750_CMD_RESET, 1, false);
    sleep_ms(10); // Pequeno atraso após reset
}

/**
 * @brief Solicita uma nova medição de luminosidade ao sensor e retorna o valor em Lux.
 * Bloqueante: aguarda o tempo de conversão do sensor.
 * @param i2c A instância do I2C a ser usada.
 * @return float - O valor da luminosidade em Lux, ou -1.0f em caso de erro na leitura.
 */
float bh1750_read_lux(i2c_inst_t *i2c) {
    uint8_t raw_data[2]; // Buffer para os 2 bytes de dados brutos
    int bytes_read;

    // Envia o comando para iniciar uma nova medição no modo de alta resolução (HIRES1).
    // O sensor automaticamente iniciará a conversão e armazenará o resultado.
    i2c_write_blocking(i2c, BH1750_ADDR, &BH1750_CMD_HIRES1, 1, false);
    
    // Aguarda o tempo de conversão do sensor.
    // Para o modo HIRES1 (0x20), o datasheet recomenda ~120ms para a conversão.
    // Usar 180ms para garantir margem de segurança.
    sleep_ms(180); 
    
    // Lê os 2 bytes do resultado da medição.
    bytes_read = i2c_read_blocking(i2c, BH1750_ADDR, raw_data, 2, false);
    if (bytes_read < 2) {
        // Retorna um valor de erro se a leitura I2C falhar (não conseguir ler 2 bytes).
        return -1.0f; 
    }
    
    // Combina os bytes lidos (MSB << 8 | LSB) para formar o valor bruto de 16 bits.
    // O datasheet especifica que o valor em Lux é (valor_bruto / 1.2).
    uint16_t raw_value = (raw_data[0] << 8) | raw_data[1];
    return (float)raw_value / 1.2f;
}