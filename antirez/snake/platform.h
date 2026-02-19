// platform.h
#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>

// Definiamo i tasti in modo universale.
// Il gioco non deve sapere se derivano da una tastiera USB o da un bottone di
// rame.
typedef enum {
  PLAT_KEY_UP = 1000,
  PLAT_KEY_DOWN,
  PLAT_KEY_LEFT,
  PLAT_KEY_RIGHT,
  PLAT_KEY_ESC,
  PLAT_KEY_NONE = 0
} PlatformKey;

// --- INIZIALIZZAZIONE E PULIZIA ---
// Configura l'hardware (Terminale su Mac, Seriale/GPIO su Pico)
void platform_init(void);

// Ripristina l'hardware all'uscita (Mostra cursore su Mac, magari non fa nulla
// su Pico)
void platform_cleanup(void);

// --- TEMPO ---
// Restituisce i microsecondi correnti dal boot/epoch
uint64_t platform_get_time_us(void);

// Mette in pausa l'esecuzione (Dorme)
void platform_sleep_us(uint64_t us);

// --- INPUT ---
// Legge l'input in modo NON bloccante. Ritorna PLAT_KEY_NONE se non c'è input.
PlatformKey platform_get_input(void);

// --- RENDER BASE ---
// Pulisce lo schermo e rimette il cursore in alto a sinistra
void platform_clear_screen(void);

// Genera un seme per la casualità
void platform_random_seed(void);

#endif // PLATFORM_H