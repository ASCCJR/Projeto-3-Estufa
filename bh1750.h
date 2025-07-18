/**
 * @file bh1750.h
 * @brief Define a interface pública (API) para o módulo do sensor de luz BH1750.
 */

#ifndef BH1750_H
#define BH1750_H

#include "hardware/i2c.h" // Incluído para o tipo i2c_inst_t

/**
 * @brief Inicializa o sensor BH1750.
 * Envia os comandos iniciais para ligar e resetar o sensor.
 * @param i2c A instância do I2C a ser usada (ex: i2c0, i2c1).
 */
void bh1750_init(i2c_inst_t *i2c);

/**
 * @brief Solicita uma nova medição de luminosidade ao sensor e retorna o valor em Lux.
 * Bloqueante: aguarda o tempo de conversão do sensor.
 * @param i2c A instância do I2C a ser usada.
 * @return float - O valor da luminosidade em Lux, ou -1.0f em caso de erro na leitura.
 */
float bh1750_read_lux(i2c_inst_t *i2c);

#endif // BH1750_H