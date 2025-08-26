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

// LED: harici LED için GPIO2; istersen onboard LED'i denemek için PICO_DEFAULT_LED_PIN kullan
#define LED_PIN            2

// LED'in yanýk kalacaðý süre (ms) - son ses aktivitesinden sonra
#define LED_HOLD_MS        120

static struct audio_buffer_pool *producer_pool;
static uint16_t current_volume = 32767;

// Callback › main loop haberleþmesi için iþaretler
static volatile uint32_t g_last_audio_ms = 0;
static volatile uint32_t cb_hits = 0;

static void core1_worker() {
    audio_i2s_set_enabled(true);
}

// *** DIKKAT: static KALDIRILDI ***
// USB ses OUT endpoint'i paket verdiðinde burasý çaðrýlacak
void _as_audio_packet(struct usb_endpoint *ep) {
    struct usb_transfer *transfer = ep->transfer;
    if (!transfer || !transfer->length) return;

    struct audio_buffer *audio_buffer = take_audio_buffer(producer_pool, false);
    if (!audio_buffer) return;

    struct usb_audio_format const *format = &audio_device_config.as_audio.format;
    uint8_t const *data = (uint8_t const *)transfer->data;

    // Örnek sayýsý (örnek baþýna bayt * kanal sayýsý ile böl)
    uint count = transfer->length / (format->nbytes_per_sample * format->nchannels);
    audio_buffer->sample_count = count;

    int16_t *out = (int16_t *)audio_buffer->buffer->bytes;
    int16_t const *in  = (int16_t const *)data;

    // Kopyala + volume uygula
    for (uint i = 0; i < count * format->nchannels; i++) {
        out[i] = (int16_t)((in[i] * current_volume) >> 15u);
    }

    // Basit peak tespiti (eþik çok düþükse gürültüye yanar, çok yüksekse hiç yanmaz)
    const int16_t THRESH = 300;
    bool has_audio = false;
    for (uint i = 0; i < count * format->nchannels; i++) {
        int16_t s = in[i];
        if (s > THRESH || s < -THRESH) { has_audio = true; break; }
    }

    if (has_audio) {
        g_last_audio_ms = to_ms_since_boot(get_absolute_time());
    }
    cb_hits++;  // teþhis: callback sayacý

    give_audio_buffer(producer_pool, audio_buffer);
}

int main() {
    stdio_init_all();

    // LED hazýrlýðý
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

    // USB ses aygýtýný baþlat
    usb_sound_card_init();

    // I2S'i core1'de çalýþtýr
    multicore_launch_core1(core1_worker);

    // Teþhis için: önceki sayaç deðeri
    uint32_t prev_hits = 0;

    while (true) {
        // 1) Callback çalýþýyor mu? Sayaç arttýysa kýsa bir "pulse" üret (LED'i hemen yak/söndürme yok)
        if (cb_hits != prev_hits) {
            prev_hits = cb_hits;
            // sade bir imleç gibi düþün: LED zaten aþaðýda "aktif ses" varsa yanacak
            // burada ekstra bir þey yapmaya gerek yok; istersen debug amaçlý kýsa bir flash ekleyebilirsin
            // gpio_put(LED_PIN, 1); sleep_ms(1); gpio_put(LED_PIN, 0);
        }

        // 2) Ses aktivitesi penceresi: son LED_HOLD_MS içinde peak görüldüyse LED ON
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if ((now - g_last_audio_ms) < LED_HOLD_MS) {
            gpio_put(LED_PIN, 1);
        } else {
            gpio_put(LED_PIN, 0);
        }

        // __wfi() LED güncellemesini geciktirebiliyor; 1 ms uyku daha akýcý
        sleep_ms(1);
    }
}

