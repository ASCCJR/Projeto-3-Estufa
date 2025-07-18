/**
 * @file aht10.h
 * @brief Arquivo de cabeçalho para o driver do sensor de temperatura e umidade I2C AHT10.
 * Declara a API pública para inicializar e ler dados do sensor.
 */

#ifndef AHT10_H
#define AHT10_H

#include "pico/stdlib.h" // Para tipos básicos como bool, float
#include "hardware/i2c.h" // Para o tipo i2c_inst_t

#define AHT10_ADDR 0x38 // Endereço I2C padrão do sensor AHT10

/**
 * @struct aht10_data_t
 * @brief Estrutura para armazenar os dados finais de temperatura e umidade lidos do sensor.
 */
typedef struct {
    float temperature; ///< Temperatura em Graus Celsius.
    float humidity;    ///< Umidade Relativa em %.
} aht10_data_t;

/**
 * @brief Inicializa o sensor AHT10.
 * Envia o comando de inicialização para o sensor.
 * @param i2c_port A instância do I2C a ser usada (ex: i2c0, i2c1).
 * @return true se o sensor foi inicializado com sucesso, false caso contrário.
 */
bool aht10_init(i2c_inst_t* i2c_port);

/**
 * @brief Dispara uma medição e lê os dados de temperatura e umidade do sensor AHT10.
 * Bloqueante: aguarda o tempo de medição do sensor.
 * @param i2c_port A instância do I2C onde o sensor está conectado.
 * @param data Ponteiro para uma estrutura aht10_data_t onde os dados lidos serão armazenados.
 * @return true se a leitura foi bem-sucedida e os dados são válidos, false caso contrário.
 */
bool aht10_read_data(i2c_inst_t* i2c_port, aht10_data_t* data);

#endif // AHT10_H