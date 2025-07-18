/**
 * @file matriz.h
 * @brief Arquivo de cabeçalho para o driver da matriz de LEDs WS2812B (Neopixel).
 */

#ifndef MATRIZ_H
#define MATRIZ_H

#include "pico/stdlib.h"

// --- Funções de Inicialização e Controle Básico ---
void matriz_init();
void matriz_limpar();

// --- Funções de Desenho de Padrões Estáticos ---
void matriz_desenhar_sol();
void matriz_desenhar_flor(uint8_t brilho);
void matriz_desenhar_ponto_central(uint8_t r, uint8_t g, uint8_t b);


// --- Funções de Animação Não-Bloqueantes ---
void matriz_iniciar_animacao_fogo(void);
void matriz_atualizar_animacao_fogo(void);
void matriz_parar_animacao_fogo(void);

void matriz_iniciar_animacao_agua(void);
bool matriz_atualizar_animacao_agua(void);

bool matriz_animacao_sol_sumindo(void);

#endif // MATRIZ_H