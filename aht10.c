/**
 * @file aht10.c
 * @brief Implementação do driver para o sensor de temperatura e umidade I2C AHT10.
 */

#include "aht10.h" // Para o próprio cabeçalho do driver

// --- Definições Internas de Comandos do Sensor ---
const uint8_t CMD_INIT[] = {0xE1, 0x08, 0x00};    // Comando de inicialização
const uint8_t CMD_MEASURE[] = {0xAC, 0x33, 0x00}; // Comando para disparar medição


// --- Implementação das Funções Públicas ---

/**
 * @brief Inicializa o sensor AHT10.
 * Envia o comando de inicialização para o sensor.
 * @param i2c A instância do I2C a ser usada.
 * @return true se o sensor foi inicializado com sucesso, false caso contrário.
 */
bool aht10_init(i2c_inst_t* i2c) {
    // Envia o comando de inicialização para o sensor.
    // Retorna false se houver erro na escrita I2C.
    int ret = i2c_write_blocking(i2c, AHT10_ADDR, CMD_INIT, sizeof(CMD_INIT), false);
    if (ret < 0) return false;
    sleep_ms(20); // Pequeno atraso para o sensor se estabilizar após a inicialização.
    return true;
}

/**
 * @brief Dispara uma medição e lê os dados de temperatura e umidade do sensor AHT10.
 * Bloqueante: aguarda o tempo de medição do sensor.
 * @param i2c A instância do I2C onde o sensor está conectado.
 * @param data Ponteiro para uma estrutura aht10_data_t onde os dados lidos serão armazenados.
 * @return true se a leitura foi bem-sucedida e os dados são válidos, false caso contrário.
 */
bool aht10_read_data(i2c_inst_t* i2c, aht10_data_t* data) {
    // 1. Envia o comando para disparar uma medição.
    // Retorna false se houver erro na escrita I2C.
    int ret = i2c_write_blocking(i2c, AHT10_ADDR, CMD_MEASURE, sizeof(CMD_MEASURE), false);
    if (ret < 0) return false;

    // 2. Espera o tempo de medição recomendado pelo datasheet (~75ms).
    sleep_ms(80);

    // 3. Lê os 6 bytes de dados de resposta do sensor.
    // Retorna false se houver erro na leitura I2C (não conseguir ler 6 bytes).
    uint8_t buf[6];
    ret = i2c_read_blocking(i2c, AHT10_ADDR, buf, sizeof(buf), false);
    if (ret < 0) return false;

    // 4. Checa o byte de status para ver se o sensor está calibrado e não está ocupado.
    // O bit 7 (status Busy/Calibrated) deve ser 0 (não ocupado) e o bit 3 (Calibrated) deve ser 1.
    if ((buf[0] & 0x88) != 0x08) {
        return false; // Retorna false se o status indica sensor ocupado ou não calibrado.
    }

    // 5. Calcula os valores de umidade e temperatura com base nas fórmulas do datasheet.
    // As fórmulas convertem os valores brutos de 20 bits (ou 20.5 bits) para as unidades reais.
    
    // Cálculo da umidade: (raw_humidity / 2^20) * 100%
    uint32_t raw_humidity = ((uint32_t)buf[1] << 12) | ((uint32_t)buf[2] << 4) | (buf[3] >> 4);
    data->humidity = ((float)raw_humidity / 1048576.0f) * 100.0f;

    // Cálculo da temperatura: ((raw_temp / 2^20) * 200) - 50
    uint32_t raw_temp = (((uint32_t)buf[3] & 0x0F) << 16) | ((uint32_t)buf[4] << 8) | buf[5];
    data->temperature = (((float)raw_temp / 1048576.0f) * 200.0f) - 50.0f;

    return true; // Leitura bem-sucedida e dados válidos.
}