/**
 * @file matriz.c
 * @brief Implementação do driver para controle da matriz de LEDs WS2812B (Neopixel).
 */

#include "matriz.h"
#include "configura_geral.h"
#include "hardware/pio.h"
#include "ws2812.pio.h"
#include <string.h>
#include <stdlib.h>
#include "pico/time.h"

// --- Definições ---
#define LED_COUNT 25
static uint32_t matriz_buffer[LED_COUNT] = {0};

// Variáveis de Animação
static bool fogo_ativo = false;
static absolute_time_t fogo_ultimo_frame_tempo;
#define FOGO_FRAME_DELAY_US 100000

static int sol_sumindo_frame_atual = 0;
static absolute_time_t sol_sumindo_ultimo_frame_tempo;
#define SOL_SUMINDO_FRAME_DELAY_US 150000

static int agua_frame_atual = 0;
static absolute_time_t agua_ultimo_frame_tempo;
#define AGUA_FRAME_DELAY_US 150000
#define AGUA_BRILHO_BASE 100

// --- Funções Auxiliares ---
static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)(g) << 16) | ((uint32_t)(r) << 8) | (uint32_t)(b);
}

static inline void put_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

static void matriz_renderizar() {
    for (int i = 0; i < LED_COUNT; ++i) {
        put_pixel(matriz_buffer[i]);
    }
}

static uint xy_to_index(uint x, uint y) {
    if (y % 2 == 0) {
        return y * 5 + x;
    } else {
        return y * 5 + (4 - x);
    }
}

// --- Funções Públicas ---
void matriz_init() {
    uint offset = pio_add_program(pio0, &ws2812_program);
    ws2812_program_init(pio0, 0, offset, MATRIZ_PIN, 800000, false);
}

void matriz_limpar() {
    memset(matriz_buffer, 0, sizeof(matriz_buffer));
    matriz_renderizar();
}

void matriz_desenhar_ponto_central(uint8_t r, uint8_t g, uint8_t b) {
    memset(matriz_buffer, 0, sizeof(matriz_buffer));
    uint32_t cor_formatada = urgb_u32(r, g, b);
    matriz_buffer[xy_to_index(2,2)] = cor_formatada;
    matriz_renderizar();
}

void matriz_desenhar_sol() {
    memset(matriz_buffer, 0, sizeof(matriz_buffer));
    uint32_t cor_sol = urgb_u32(255, 200, 0);
    matriz_buffer[xy_to_index(2, 1)] = cor_sol;
    matriz_buffer[xy_to_index(1, 2)] = cor_sol;
    matriz_buffer[xy_to_index(2, 2)] = cor_sol;
    matriz_buffer[xy_to_index(3, 2)] = cor_sol;
    matriz_buffer[xy_to_index(2, 3)] = cor_sol;
    matriz_buffer[xy_to_index(0, 0)] = cor_sol;
    matriz_buffer[xy_to_index(4, 0)] = cor_sol;
    matriz_buffer[xy_to_index(0, 4)] = cor_sol;
    matriz_buffer[xy_to_index(4, 4)] = cor_sol;
    matriz_renderizar();
}

void matriz_desenhar_flor(uint8_t brilho) {
    memset(matriz_buffer, 0, sizeof(matriz_buffer));
    uint32_t cor_vermelha_petala = urgb_u32(50 * brilho / 50, 0, 0);
    uint32_t cor_verde_caule = urgb_u32(0, 50 * brilho / 50, 0);
    matriz_buffer[xy_to_index(4, 0)] = cor_verde_caule;
    matriz_buffer[xy_to_index(3, 1)] = cor_verde_caule;
    matriz_buffer[xy_to_index(2, 2)] = cor_verde_caule;
    matriz_buffer[xy_to_index(1, 3)] = cor_verde_caule;
    matriz_buffer[xy_to_index(0, 2)] = cor_vermelha_petala;
    matriz_buffer[xy_to_index(1, 2)] = cor_vermelha_petala;
    matriz_buffer[xy_to_index(0, 3)] = cor_vermelha_petala;
    matriz_buffer[xy_to_index(2, 3)] = cor_vermelha_petala;
    matriz_buffer[xy_to_index(0, 4)] = cor_vermelha_petala;
    matriz_buffer[xy_to_index(1, 4)] = cor_vermelha_petala;
    matriz_buffer[xy_to_index(2, 4)] = cor_vermelha_petala;
    matriz_renderizar();
}


bool matriz_animacao_sol_sumindo() {
    if (sol_sumindo_frame_atual == 0) {
        memset(matriz_buffer, 0, sizeof(matriz_buffer));
        sol_sumindo_ultimo_frame_tempo = get_absolute_time();
        matriz_desenhar_sol();
        sol_sumindo_frame_atual++;
        return false;
    }
    if (absolute_time_diff_us(sol_sumindo_ultimo_frame_tempo, get_absolute_time()) < SOL_SUMINDO_FRAME_DELAY_US) return false;
    sol_sumindo_ultimo_frame_tempo = get_absolute_time();
    switch (sol_sumindo_frame_atual) {
        case 1:
            matriz_buffer[xy_to_index(0, 0)] = 0;
            matriz_buffer[xy_to_index(4, 0)] = 0;
            matriz_buffer[xy_to_index(0, 4)] = 0;
            matriz_buffer[xy_to_index(4, 4)] = 0;
            break;
        case 2:
            matriz_buffer[xy_to_index(2, 1)] = 0;
            matriz_buffer[xy_to_index(2, 3)] = 0;
            matriz_buffer[xy_to_index(1, 2)] = 0;
            matriz_buffer[xy_to_index(3, 2)] = 0;
            break;
        case 3:
            matriz_buffer[xy_to_index(2, 2)] = 0;
            break;
        case 4:
            sol_sumindo_frame_atual = 0;
            matriz_limpar();
            return true;
        default:
            sol_sumindo_frame_atual = 0;
            return true;
    }
    matriz_renderizar();
    sol_sumindo_frame_atual++;
    return false;
}

void matriz_iniciar_animacao_agua(void) {
    agua_frame_atual = 0;
    matriz_limpar();
    agua_ultimo_frame_tempo = get_absolute_time();
}

bool matriz_atualizar_animacao_agua(void) {
    if (absolute_time_diff_us(agua_ultimo_frame_tempo, get_absolute_time()) < AGUA_FRAME_DELAY_US) return false;
    agua_ultimo_frame_tempo = get_absolute_time();
    uint32_t cor_agua = urgb_u32(0, 0, AGUA_BRILHO_BASE);
    memset(matriz_buffer, 0, sizeof(matriz_buffer));
    if (agua_frame_atual < 5) {
        matriz_buffer[xy_to_index(1, agua_frame_atual)] = cor_agua;
        matriz_buffer[xy_to_index(2, agua_frame_atual)] = cor_agua;
        matriz_buffer[xy_to_index(3, agua_frame_atual)] = cor_agua;
        if (agua_frame_atual > 0) {
             matriz_buffer[xy_to_index(1, agua_frame_atual - 1)] = urgb_u32(0,0,AGUA_BRILHO_BASE / 2);
             matriz_buffer[xy_to_index(2, agua_frame_atual - 1)] = urgb_u32(0,0,AGUA_BRILHO_BASE / 2);
             matriz_buffer[xy_to_index(3, agua_frame_atual - 1)] = urgb_u32(0,0,AGUA_BRILHO_BASE / 2);
        }
    }
    matriz_renderizar();
    agua_frame_atual++;
    if (agua_frame_atual >= 6) {
        agua_frame_atual = 0;
        return true;
    }
    return false;
}