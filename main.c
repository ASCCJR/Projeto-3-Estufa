/**
 * @file main.c
 * @brief Gerenciamento principal do sistema da Estufa Inteligente.
 */

#include "configura_geral.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "pico/multicore.h"
#include "pico/cyw43_arch.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/i2c.h"
#include "pico/time.h"

#include "display.h"
#include "mqtt_lwip.h"
#include "matriz.h"
#include "rgb_led.h"
#include "buzzer.h"
#include "servo.h"
#include "aht10.h"
#include "bh1750.h"

/* Estruturas de Dados */
typedef struct {
    bool ativo;
    absolute_time_t inicio;
    uint64_t duracao_us;
} TimerNaoBloqueante;

typedef struct {
    volatile enum ModoOperacao modo_atual;
    bool modo_foi_inicializado;

    TimerNaoBloqueante timer_geral;
    TimerNaoBloqueante timer_leitura_sensor;
    TimerNaoBloqueante timer_irrigador_servo;
    TimerNaoBloqueante timer_display_update;

    bool alarme_luminosidade_ativo;
    bool irrigador_servo_posicao_atual;

    absolute_time_t ultimo_tempo_botao_b;
    TimerNaoBloqueante timer_heartbeat;
} EstadoSistema;

/* Variáveis Globais */
static EstadoSistema sistema;
static aht10_data_t dados_sensor;
static float dados_luminosidade;

/* Protótipos de Funções Estaduais */
void handle_modo_estufa_ok();
void handle_modo_alerta_luz();
void handle_modo_protegendo();
void handle_modo_protegido();
void handle_modo_irrigacao();
void handle_modo_msg_irrigacao_fim();

/* Protótipos de Funções Auxiliares */
void timer_iniciar(TimerNaoBloqueante *timer, uint64_t duracao_us);
bool timer_expirou(TimerNaoBloqueante *timer);
void rgb_led_desligar();
void solicitar_publicacao_mqtt(enum MQTT_MSG_TYPE tipo_msg);
void verificar_fifo();
void inicia_hardware();
void inicia_core1();
void funcao_wifi_nucleo1();


/* --- Implementação das Funções --- */

/**
 * @brief Inicia um timer não-bloqueante.
 * @param timer Ponteiro para a estrutura do timer.
 * @param duracao_us Duração do timer em microssegundos.
 */
void timer_iniciar(TimerNaoBloqueante *timer, uint64_t duracao_us) {
    timer->ativo = true;
    timer->inicio = get_absolute_time();
    timer->duracao_us = duracao_us;
}

/**
 * @brief Verifica se um timer não-bloqueante expirou.
 * @param timer Ponteiro para a estrutura do timer.
 * @return true se o timer expirou, false caso contrário.
 */
bool timer_expirou(TimerNaoBloqueante *timer) {
    if (!timer->ativo) return false;
    if (absolute_time_diff_us(timer->inicio, get_absolute_time()) >= timer->duracao_us) {
        timer->ativo = false;
        return true;
    }
    return false;
}

/**
 * @brief Desliga o LED RGB.
 */
void rgb_led_desligar() {
    rgb_led_set_color(0, 0, 0);
}

/**
 * @brief Solicita uma publicação MQTT para o Core1 via FIFO.
 * @param tipo_msg O tipo de mensagem MQTT a ser publicada.
 */
void solicitar_publicacao_mqtt(enum MQTT_MSG_TYPE tipo_msg) {
    uint32_t pacote = (FIFO_CMD_PUBLICAR_MQTT << 16) | (tipo_msg & 0xFF);
    multicore_fifo_push_blocking(pacote);
}

/**
 * @brief Verifica o FIFO do multicore para comandos do Core1.
 */
void verificar_fifo() {
    if (multicore_fifo_rvalid()) {
        uint32_t pacote = multicore_fifo_pop_blocking();
        uint16_t comando = pacote >> 16;
        uint16_t valor = pacote & 0xFFFF;
        if (comando == FIFO_CMD_MUDAR_ESTADO) {
            sistema.modo_atual = (enum ModoOperacao)valor;
            sistema.modo_foi_inicializado = false;
        }
    }
}

/**
 * @brief Manipula a lógica quando o sistema está no modo de Estufa OK.
 * Monitora luminosidade e transiciona para alerta se necessário.
 */
void handle_modo_estufa_ok() {
    if (!sistema.modo_foi_inicializado) {
        char linha2[25], linha3[25];
        sprintf(linha2, "Temp: %.1f C", dados_sensor.temperature);
        sprintf(linha3, "Luz: %.0f lux", dados_luminosidade);
        display_show_message("Estufa OK", linha2, linha3);
        rgb_led_set_color(0, PWM_MAX_DUTY, 0); // Verde
        sistema.modo_foi_inicializado = true;
    }
    matriz_desenhar_flor(100);
    if (dados_luminosidade > LUZ_MAXIMA_ESTUFA && !sistema.alarme_luminosidade_ativo) {
        sistema.alarme_luminosidade_ativo = true;
        solicitar_publicacao_mqtt(MSG_ALARM_LUZ_ON);
        sistema.modo_atual = MODO_ESTUFA_ALERTA_LUZ;
        sistema.modo_foi_inicializado = false;
    }
}

/**
 * @brief Manipula a lógica quando o sistema está em alerta de luminosidade alta.
 */
void handle_modo_alerta_luz() {
    if (!sistema.modo_foi_inicializado) {
        display_show_message("ALERTA!", "Luminosidade ALTA", "");
        matriz_desenhar_sol();
        buzzer_play_tone(1500, 200);
        timer_iniciar(&sistema.timer_geral, TEMPO_MSG_BEM_VINDO_US);
        sistema.modo_foi_inicializado = true;
    }
    if (timer_expirou(&sistema.timer_geral)) {
        sistema.modo_atual = MODO_ESTUFA_PROTEGENDO;
        sistema.modo_foi_inicializado = false;
    }
}

/**
 * @brief Manipula a lógica quando o sistema está ativando a proteção solar.
 * Exibe uma animação do sol sumindo.
 */
void handle_modo_protegendo() {
    if (!sistema.modo_foi_inicializado) {
        display_show_message("Acao Corretiva", "Ativando protecao", "solar...");
        sistema.modo_foi_inicializado = true;
    }
    if (matriz_animacao_sol_sumindo()) {
        sistema.modo_atual = MODO_ESTUFA_PROTEGIDO;
        sistema.modo_foi_inicializado = false;
    }
}

/**
 * @brief Manipula a lógica quando o sistema está no modo protegido.
 * Aguarda a luminosidade retornar ao normal.
 */
void handle_modo_protegido() {
    if (!sistema.modo_foi_inicializado) {
        display_show_message("Sistema em Alerta", "Protecao Ativada", "");
        matriz_limpar();
        rgb_led_set_color(PWM_MAX_DUTY, PWM_MAX_DUTY, 0); // Amarelo
        sistema.modo_foi_inicializado = true;
    }
    if (dados_luminosidade <= LUZ_MAXIMA_ESTUFA && sistema.alarme_luminosidade_ativo) {
        sistema.alarme_luminosidade_ativo = false;
        solicitar_publicacao_mqtt(MSG_ALARM_LUZ_OFF);
        sistema.modo_atual = MODO_ESTUFA_OK;
        sistema.modo_foi_inicializado = false;
    }
}

/**
 * @brief Manipula a lógica quando o sistema está no modo de irrigação.
 */
void handle_modo_irrigacao() {
    if (!sistema.modo_foi_inicializado) {
        display_show_message("Irrigacao Ativada", "Iniciando...", NULL);
        rgb_led_set_color(0, 0, PWM_MAX_DUTY); // Azul
        matriz_iniciar_animacao_agua();
        timer_iniciar(&sistema.timer_geral, 10 * 1000000); // 10s
        sistema.modo_foi_inicializado = true;
        servo_start_move(30);
        timer_iniciar(&sistema.timer_irrigador_servo, 1500000);
        sistema.irrigador_servo_posicao_atual = true;
        timer_iniciar(&sistema.timer_display_update, 1000000);
    }
    matriz_atualizar_animacao_agua();
    if (timer_expirou(&sistema.timer_irrigador_servo)) {
        servo_start_move(sistema.irrigador_servo_posicao_atual ? 150 : 30);
        sistema.irrigador_servo_posicao_atual = !sistema.irrigador_servo_posicao_atual;
        timer_iniciar(&sistema.timer_irrigador_servo, 1500000);
    }
    if (timer_expirou(&sistema.timer_display_update) || !sistema.timer_display_update.ativo) {
        int64_t diff_us = absolute_time_diff_us(sistema.timer_geral.inicio, get_absolute_time());
        int tempo_restante_s = 10 - (diff_us / 1000000);
        if (tempo_restante_s < 0) tempo_restante_s = 0;
        char linha2_display[25];
        sprintf(linha2_display, "Restam: %ds", tempo_restante_s);
        display_show_message("Irrigacao Ativada", linha2_display, NULL);
        timer_iniciar(&sistema.timer_display_update, 1000000);
    }
    if (timer_expirou(&sistema.timer_geral)) {
        rgb_led_desligar();
        matriz_limpar();
        servo_stop_move();
        sistema.timer_irrigador_servo.ativo = false;
        sistema.modo_atual = MODO_MSG_IRRIGACAO_FIM;
        sistema.modo_foi_inicializado = false;
    }
}

/**
 * @brief Manipula a lógica para exibir a mensagem de "Irrigação Finalizada".
 */
void handle_modo_msg_irrigacao_fim() {
    if (!sistema.modo_foi_inicializado) {
        display_show_message("Irrigacao Finalizada", NULL, NULL);
        timer_iniciar(&sistema.timer_geral, TEMPO_MSG_IRRIGACAO_FIM_US);
        sistema.modo_foi_inicializado = true;
    }
    if (timer_expirou(&sistema.timer_geral)) {
        sistema.modo_atual = MODO_ESTUFA_OK;
        sistema.modo_foi_inicializado = false;
    }
}

/**
 * @brief Inicializa todo o hardware necessário para o sistema.
 */
void inicia_hardware() {
    stdio_init_all();
    display_init();
    rgb_led_init();
    buzzer_init();
    servo_init();
    matriz_init();
    matriz_limpar();

    gpio_init(BOTAO_B_PIN);
    gpio_set_dir(BOTAO_B_PIN, GPIO_IN);
    gpio_pull_up(BOTAO_B_PIN);

    i2c_init(i2c0, 100 * 1000);
    gpio_set_function(I2C0_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C0_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C0_SDA_PIN);
    gpio_pull_up(I2C0_SCL_PIN);
    
    if (!aht10_init(i2c0)) {
        display_show_message("ERRO FATAL", "AHT10 falhou!", NULL);
        while (true) tight_loop_contents();
    }
    bh1750_init(i2c0); // BH1750 não tem checagem de retorno na init

    memset(&sistema, 0, sizeof(EstadoSistema));
    sistema.modo_atual = MODO_ESTUFA_OK;
}

/**
 * @brief Lança o Core1 para executar a função Wi-Fi.
 */
void inicia_core1() {
    multicore_launch_core1(funcao_wifi_nucleo1);
}

/**
 * @brief Função principal do sistema (Core0). Gerencia a máquina de estados,
 * leitura de sensores e interação com o usuário.
 */
int main() {
    inicia_hardware();
    display_show_message("Rede", "Conectando Wi-Fi...", NULL);
    inicia_core1();

    uint32_t fifo_response;
    while (!multicore_fifo_rvalid()) { tight_loop_contents(); }
    fifo_response = multicore_fifo_pop_blocking();
    if ((fifo_response >> 16) != FIFO_CMD_WIFI_CONECTADO || (fifo_response & 0xFFFF) != WIFI_STATUS_SUCCESS) {
        display_show_message("ERRO FATAL", "Falha no Wi-Fi", NULL);
        while(true) tight_loop_contents();
    }
    
    display_show_message("Rede", "Conectando MQTT...", NULL);
    while(true) {
        if (multicore_fifo_rvalid()) {
            fifo_response = multicore_fifo_pop_blocking();
            if ((fifo_response >> 16) == FIFO_CMD_MQTT_CONECTADO) break;
        }
        tight_loop_contents();
    }

    display_show_message("BitDogEstufa", "Sistema Pronto", NULL);
    buzzer_tocar_melodia_sucesso();
    sleep_ms(2500);

    while (true) {
        verificar_fifo();

        // Botão B: Alterna irrigação
        static bool btn_b_pressed = false;
        if (!gpio_get(BOTAO_B_PIN) && !btn_b_pressed) {
            btn_b_pressed = true;
            if (sistema.modo_atual != MODO_ESTUFA_IRRIGACAO) {
                sistema.modo_atual = MODO_ESTUFA_IRRIGACAO;
            } else { // Se já está irrigando, cancela e volta para OK
                sistema.modo_atual = MODO_ESTUFA_OK;
            }
            sistema.modo_foi_inicializado = false;
        } else if (gpio_get(BOTAO_B_PIN)) {
            btn_b_pressed = false;
        }
        
        // Leitura de Sensores
        if (timer_expirou(&sistema.timer_leitura_sensor) || !sistema.timer_leitura_sensor.ativo) {
            if (aht10_read_data(i2c0, &dados_sensor)) {
                uint16_t temp_int = (uint16_t)(dados_sensor.temperature * 100.0f);
                multicore_fifo_push_blocking((FIFO_CMD_PUB_SENSOR_TEMP << 16) | temp_int);
                uint16_t umid_int = (uint16_t)(dados_sensor.humidity * 100.0f);
                multicore_fifo_push_blocking((FIFO_CMD_PUB_SENSOR_UMID << 16) | umid_int);
            }
            float lux = bh1750_read_lux(i2c0);
            if (lux >= 0) {
                dados_luminosidade = lux;
                multicore_fifo_push_blocking((FIFO_CMD_PUB_SENSOR_LUZ << 16) | (uint16_t)lux);
            }
            timer_iniciar(&sistema.timer_leitura_sensor, 10000000); // Leitura a cada 10s
        }

        // Máquina de Estados
        switch (sistema.modo_atual) {
            case MODO_ESTUFA_OK: handle_modo_estufa_ok(); break;
            case MODO_ESTUFA_ALERTA_LUZ: handle_modo_alerta_luz(); break;
            case MODO_ESTUFA_PROTEGENDO: handle_modo_protegendo(); break;
            case MODO_ESTUFA_PROTEGIDO: handle_modo_protegido(); break;
            case MODO_ESTUFA_IRRIGACAO: handle_modo_irrigacao(); break;
            case MODO_MSG_IRRIGACAO_FIM: handle_modo_msg_irrigacao_fim(); break;
            default:
                sistema.modo_atual = MODO_ESTUFA_OK;
                sistema.modo_foi_inicializado = false;
                break;
        }

        // Heartbeat
        if (timer_expirou(&sistema.timer_heartbeat) || !sistema.timer_heartbeat.ativo) {
            solicitar_publicacao_mqtt(MSG_LOG_HEARTBEAT);
            timer_iniciar(&sistema.timer_heartbeat, 30000000);
        }
        tight_loop_contents();
    }
    return 0;
}

/**
 * @brief Função executada no Core1, responsável pela comunicação Wi-Fi e MQTT.
 */
void funcao_wifi_nucleo1() {
    #define QUEUE_SIZE 10
    typedef struct { char topico[100]; char mensagem[100]; } publication_t;
    static publication_t publication_queue[QUEUE_SIZE];
    static int queue_head = 0, queue_tail = 0;
    static TimerNaoBloqueante timer_entre_publicacoes;

    cyw43_arch_init();
    cyw43_arch_enable_sta_mode();
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        multicore_fifo_push_blocking((FIFO_CMD_WIFI_CONECTADO << 16) | WIFI_STATUS_FAIL);
    } else {
        multicore_fifo_push_blocking((FIFO_CMD_WIFI_CONECTADO << 16) | WIFI_STATUS_SUCCESS);
    }
    iniciar_mqtt_cliente();

    while (true) {
        if (multicore_fifo_rvalid()) {
            uint32_t pacote = multicore_fifo_pop_blocking();
            uint16_t comando = pacote >> 16;
            char msg_buffer[100], base_topic[100];
            
            if (comando == FIFO_CMD_PUBLICAR_MQTT) {
                uint8_t tipo_msg = pacote & 0xFF;
                switch ((enum MQTT_MSG_TYPE)tipo_msg) {
                    case MSG_ALARM_LUZ_ON: strcpy(base_topic, "alarme"); strcpy(msg_buffer, "{\"alarme\":\"luminosidade\", \"status\":\"ativo\"}"); break;
                    case MSG_ALARM_LUZ_OFF: strcpy(base_topic, "alarme"); strcpy(msg_buffer, "{\"alarme\":\"luminosidade\", \"status\":\"ok\"}"); break;
                    case MSG_LOG_HEARTBEAT: strcpy(base_topic, TOPICO_HEARTBEAT); strcpy(msg_buffer, "ok"); break;
                }
                 int next_tail = (queue_tail + 1) % QUEUE_SIZE;
                if (next_tail != queue_head) {
                    snprintf(publication_queue[queue_tail].topico, sizeof(publication_queue[queue_tail].topico), "%s/%s", DEVICE_ID, base_topic);
                    strncpy(publication_queue[queue_tail].mensagem, msg_buffer, sizeof(publication_queue[queue_tail].mensagem) - 1);
                    publication_queue[queue_tail].mensagem[sizeof(publication_queue[queue_tail].mensagem) - 1] = '\0';
                    queue_tail = next_tail;
                }
            } else if (comando >= FIFO_CMD_PUB_SENSOR_TEMP && comando <= FIFO_CMD_PUB_SENSOR_LUZ) {
                uint16_t valor_int = pacote & 0xFFFF;
                char topico_final[100], msg_final[20];
                if (comando == FIFO_CMD_PUB_SENSOR_TEMP) {
                    sprintf(msg_final, "%.2f", (float)valor_int / 100.0f);
                    snprintf(topico_final, sizeof(topico_final), "%s/sensores/temperatura", DEVICE_ID);
                } else if (comando == FIFO_CMD_PUB_SENSOR_UMID) {
                    sprintf(msg_final, "%.2f", (float)valor_int / 100.0f);
                    snprintf(topico_final, sizeof(topico_final), "%s/sensores/umidade", DEVICE_ID);
                } else { // FIFO_CMD_PUB_SENSOR_LUZ
                    sprintf(msg_final, "%d", valor_int);
                    snprintf(topico_final, sizeof(topico_final), "%s/sensores/luminosidade", DEVICE_ID);
                }
                int next_tail = (queue_tail + 1) % QUEUE_SIZE;
                if (next_tail != queue_head) {
                    strncpy(publication_queue[queue_tail].topico, topico_final, sizeof(publication_queue[queue_tail].topico));
                    strncpy(publication_queue[queue_tail].mensagem, msg_final, sizeof(publication_queue[queue_tail].mensagem));
                    queue_tail = next_tail;
                }
            }
        }
        if (!mqtt_is_publishing() && queue_head != queue_tail && (timer_expirou(&timer_entre_publicacoes) || !timer_entre_publicacoes.ativo)) {
            publication_t *pub = &publication_queue[queue_head];
            publicar_mensagem_mqtt(pub->topico, pub->mensagem);
            queue_head = (queue_head + 1) % QUEUE_SIZE;
            timer_iniciar(&timer_entre_publicacoes, 50000);
        }
        cyw43_arch_poll();
        sleep_ms(1);
    }
}