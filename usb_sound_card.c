#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/audio.h"
#include "pico/audio_i2s.h"
#include "pico/audio_spdif.h"
#include "pico/audio_usb.h"
#include "hardware/gpio.h"
#include "pico/time.h"   // to_ms_since_boot

#define AUDIO_BUFFER_COUNT 8
#define AUDIO_BUFFER_SIZE  192

// LED: harici LED i�in GPIO2; istersen onboard LED'i denemek i�in PICO_DEFAULT_LED_PIN kullan
#define LED_PIN            2

// LED'in yan�k kalaca�� s�re (ms) - son ses aktivitesinden sonra
#define LED_HOLD_MS        120

static struct audio_buffer_pool *producer_pool;
static uint16_t current_volume = 32767;

// Callback � main loop haberle�mesi i�in i�aretler
static volatile uint32_t g_last_audio_ms = 0;
static volatile uint32_t cb_hits = 0;

static void core1_worker() {
    audio_i2s_set_enabled(true);
}

// *** DIKKAT: static KALDIRILDI ***
// USB ses OUT endpoint'i paket verdi�inde buras� �a�r�lacak
void _as_audio_packet(struct usb_endpoint *ep) {
    struct usb_transfer *transfer = ep->transfer;
    if (!transfer || !transfer->length) return;

    struct audio_buffer *audio_buffer = take_audio_buffer(producer_pool, false);
    if (!audio_buffer) return;

    struct usb_audio_format const *format = &audio_device_config.as_audio.format;
    uint8_t const *data = (uint8_t const *)transfer->data;

    // �rnek say�s� (�rnek ba��na bayt * kanal say�s� ile b�l)
    uint count = transfer->length / (format->nbytes_per_sample * format->nchannels);
    audio_buffer->sample_count = count;

    int16_t *out = (int16_t *)audio_buffer->buffer->bytes;
    int16_t const *in  = (int16_t const *)data;

    // Kopyala + volume uygula
    for (uint i = 0; i < count * format->nchannels; i++) {
        out[i] = (int16_t)((in[i] * current_volume) >> 15u);
    }

    // Basit peak tespiti (e�ik �ok d���kse g�r�lt�ye yanar, �ok y�ksekse hi� yanmaz)
    const int16_t THRESH = 300;
    bool has_audio = false;
    for (uint i = 0; i < count * format->nchannels; i++) {
        int16_t s = in[i];
        if (s > THRESH || s < -THRESH) { has_audio = true; break; }
    }

    if (has_audio) {
        g_last_audio_ms = to_ms_since_boot(get_absolute_time());
    }
    cb_hits++;  // te�his: callback sayac�

    give_audio_buffer(producer_pool, audio_buffer);
}

int main() {
    stdio_init_all();

    // LED haz�rl���
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, true);
    gpio_put(LED_PIN, 0);

    set_sys_clock_48mhz();
    puts("USB SOUND CARD with LED (diagnostic)");

    // 48 kHz, 16-bit stereo
    struct audio_format audio_format_48k = {
        .format = AUDIO_BUFFER_FORMAT_PCM_S16,
        .sample_freq = 48000,
        .channel_count = 2,
    };

    struct audio_buffer_format producer_format = {
        .format = &audio_format_48k,
        .sample_stride = 4, // 16-bit stereo -> 4 byte
    };

    producer_pool = audio_new_producer_pool(&producer_format, AUDIO_BUFFER_COUNT, AUDIO_BUFFER_SIZE);

    struct audio_i2s_config config = {
        .data_pin = PICO_AUDIO_I2S_DATA_PIN,
        .clock_pin_base = PICO_AUDIO_I2S_CLOCK_PIN_BASE,
        .dma_channel = 0,
        .pio_sm = 0,
    };

    const struct audio_format *output_format = audio_i2s_setup(&audio_format_48k, &config);
    if (!output_format) {
        panic("Failed to open audio device");
    }

    bool ok = audio_i2s_connect_extra(producer_pool, false, 2, 96, NULL);
    if (!ok) panic("audio_i2s_connect_extra failed");

    // USB ses ayg�t�n� ba�lat
    usb_sound_card_init();

    // I2S'i core1'de �al��t�r
    multicore_launch_core1(core1_worker);

    // Te�his i�in: �nceki saya� de�eri
    uint32_t prev_hits = 0;

    while (true) {
        // 1) Callback �al���yor mu? Saya� artt�ysa k�sa bir "pulse" �ret (LED'i hemen yak/s�nd�rme yok)
        if (cb_hits != prev_hits) {
            prev_hits = cb_hits;
            // sade bir imle� gibi d���n: LED zaten a�a��da "aktif ses" varsa yanacak
            // burada ekstra bir �ey yapmaya gerek yok; istersen debug ama�l� k�sa bir flash ekleyebilirsin
            // gpio_put(LED_PIN, 1); sleep_ms(1); gpio_put(LED_PIN, 0);
        }

        // 2) Ses aktivitesi penceresi: son LED_HOLD_MS i�inde peak g�r�ld�yse LED ON
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if ((now - g_last_audio_ms) < LED_HOLD_MS) {
            gpio_put(LED_PIN, 1);
        } else {
            gpio_put(LED_PIN, 0);
        }

        // __wfi() LED g�ncellemesini geciktirebiliyor; 1 ms uyku daha ak�c�
        sleep_ms(1);
    }
}

