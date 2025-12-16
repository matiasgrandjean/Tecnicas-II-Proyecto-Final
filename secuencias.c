// secuencias.c
#include "secuencias.h"
#include "nocanonico.h"

#include <wiringPi.h>
#include <wiringSerial.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <string.h>

// Variables definidas en main.c (compartidas porque se incluye este .c allí)
extern int serial_fd;
extern int modoRemoto;

// -------------------- Parámetros globales de control de velocidad --------------------
const int pasoDelay      = 50;    // Paso de ajuste (ms)
const int delayMin       = 50;    // Límite inferior (más rápido)
const int delayMax       = 2000;  // Límite superior (más lento)
const int pasoSubDelay   = 10;    // Paso de subdelay (ms) para respuesta rápida

// Velocidades guardadas por secuencia (persisten entre llamadas)
static int vel_auto      = 0;
static int vel_choque    = 0;
static int vel_apilada   = 0;
static int vel_carrera   = 0;
static int vel_binario   = 0;
static int vel_danza     = 0;
static int vel_firstinfirstoff  = 0;
static int vel_escalera  = 0;

// Definición de LEDs
const unsigned char LEDS[8] = {23, 24, 25, 12, 16, 20, 21, 26};

// Tablas de secuencias
// "La carrera" (tabla de datos)
const unsigned char carrera[][8] = {
    {1,0,0,0,0,0,0,0}, {0,1,0,0,0,0,0,0}, {0,0,1,0,0,0,0,0},
    {1,0,0,1,0,0,0,0}, {0,1,0,0,1,0,0,0}, {0,0,1,0,1,0,0,0},
    {0,0,0,1,0,1,0,0}, {0,0,0,0,1,1,0,0}, {0,0,0,0,0,1,1,0},
    {0,0,0,0,0,0,1,0}, {0,0,0,0,0,0,0,1}
};

// "El choque" (tabla de datos)
const unsigned char choque[][8] = {
    {1,0,0,0,0,0,0,1}, {0,1,0,0,0,0,1,0}, {0,0,1,0,0,1,0,0},
    {0,0,0,1,1,0,0,0}, {0,0,0,1,1,0,0,0}, {0,0,1,0,0,1,0,0},
    {0,1,0,0,0,0,1,0}, {1,0,0,0,0,0,0,1}
};

// "Salto intermedio" (tabla de datos: LED de por medio)
const unsigned char danza[][8] = {
    {0,0,1,1,0,0,1,1},
    {1,1,0,0,1,1,0,0},
    {0,0,1,1,1,1,0,0},
    {1,1,0,0,0,0,1,1},
    {1,1,1,1,1,1,1,1},
    {1,1,0,0,0,0,1,1},
    {0,0,1,1,1,1,0,0},
    {1,1,0,0,1,1,0,0},
    {0,0,1,1,0,0,1,1},
};

// "Escalera central" (tabla de datos: se llena hacia el centro y vuelve)
const unsigned char escaleraCentral[][8] = {
    {0,0,0,0,0,0,0,0},
    {1,0,0,0,0,0,0,1},
    {1,1,0,0,0,0,1,1},
    {1,1,1,0,0,1,1,1},
    {1,1,1,1,1,1,1,1},
    {1,1,1,0,0,1,1,1},
    {1,1,0,0,0,0,1,1},
    {1,0,0,0,0,0,0,1}
};

const int cantEstados_Carrera          = sizeof(carrera) / sizeof(carrera[0]);
const int cantEstados_Choque           = sizeof(choque) / sizeof(choque[0]);
const int cantEstados_Danza            = sizeof(danza) / sizeof(danza[0]);
const int cantEstados_EscaleraCentral  = sizeof(escaleraCentral) / sizeof(escaleraCentral[0]);

// Funciones auxiliares

static void apagarLeds(void) {
    for (int i = 0; i < 8; i++)
        digitalWrite(LEDS[i], LOW);
}

static void aplicarEstado(const unsigned char frame[8]) {
    for (int j = 0; j < 8; j++)
        digitalWrite(LEDS[j], frame[j]);
}

// Maneja teclado o UART:
// - LOCAL: flechas ↑/↓ ajustan delay, 'q' sale.
// - REMOTO: flechas ↑/↓ (enviadas por el terminal) ajustan delay, 'q' sale.
static int manejarTeclado(struct termios *orig_t, int orig_flags, int *delay_ms) {
    char c;
    ssize_t n;

    // MODO LOCAL
    if (!modoRemoto) {
        static unsigned int ultimoCambioTiempo = 0;
        const unsigned int intervaloTiempo = 80; // ms mínimos entre cambios grandes

        while (1) {
            n = read(STDIN_FILENO, &c, 1);

            if (n == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    break; // No hay más datos
                else
                    break; // Otro error
            } else if (n == 0) {
                break; // EOF o sin datos
            }

            // Salir con 'q'
            if (c == 'q' || c == 'Q') {
                restaurarTerminal(orig_t, orig_flags);
                apagarLeds();
                return 1;
            }

            // Flechas ESC [ A / ESC [ B
            if (c == 27) { // ESC
                char seq[2];
                ssize_t n1 = read(STDIN_FILENO, &seq[0], 1);
                ssize_t n2 = read(STDIN_FILENO, &seq[1], 1);

                if (n1 == 1 && n2 == 1 && seq[0] == '[') {
                    unsigned int tiempoActual = millis();
                    if (tiempoActual - ultimoCambioTiempo < intervaloTiempo) {
                        continue;
                    }
                    ultimoCambioTiempo = tiempoActual;

                    if (seq[1] == 'A') {        // flecha arriba
                        *delay_ms -= pasoDelay;
                        if (*delay_ms < delayMin)
                            *delay_ms = delayMin;
                    } else if (seq[1] == 'B') { // flecha abajo
                        *delay_ms += pasoDelay;
                        if (*delay_ms > delayMax)
                            *delay_ms = delayMax;
                    }
                }
            }
        }

        return 0;
    }

    // MODO REMOTO (PC via UART)
    if (serial_fd < 0)
        return 0;

    static unsigned int ultimoCambioTiempo_Remoto = 0;
    const unsigned int intervaloTiempo_Remoto = 80;

    while (serialDataAvail(serial_fd)) {
        c = (char)serialGetchar(serial_fd);

        if (c == 'q' || c == 'Q') {
            restaurarTerminal(orig_t, orig_flags);
            apagarLeds();
            return 1;
        }

        if (c == 27) { // ESC
            char seq[2];
            // Intentamos leer los siguientes dos bytes si están disponibles
            ssize_t n1 = 0, n2 = 0;
            if (serialDataAvail(serial_fd)) {
                seq[0] = (char)serialGetchar(serial_fd);
                n1 = 1;
            }
            if (serialDataAvail(serial_fd)) {
                seq[1] = (char)serialGetchar(serial_fd);
                n2 = 1;
            }

            if (n1 == 1 && n2 == 1 && seq[0] == '[') {
                unsigned int tiempoActual = millis();
                if (tiempoActual - ultimoCambioTiempo_Remoto < intervaloTiempo_Remoto) {
                    continue;
                }
                ultimoCambioTiempo_Remoto = tiempoActual;

                if (seq[1] == 'A') {        // flecha arriba
                    *delay_ms -= pasoDelay;
                    if (*delay_ms < delayMin)
                        *delay_ms = delayMin;
                } else if (seq[1] == 'B') { // flecha abajo
                    *delay_ms += pasoDelay;
                    if (*delay_ms > delayMax)
                        *delay_ms = delayMax;
                }
            }
        }
    }

    return 0;
}

// Espera con subdelays para respuesta inmediata al teclado
static int delayInteligente(int total_ms, struct termios *orig_t, int orig_flags, int *delay_ms) {
    if (total_ms < pasoSubDelay)
        total_ms = pasoSubDelay;

    // Para remoto: solo actualiza cuando cambia y evita spamear el UART
    int ultimaVelocidadMostrada = -1;

    for (int t = 0; t < total_ms; t += pasoSubDelay) {

        if (!modoRemoto) {
            printf("\rDelay secuencia: %d ms - Velocidad secuencia: %.2f Hz   ", *delay_ms, 1000.0 / (double)(*delay_ms));
            fflush(stdout);
        }

        // En modo remoto, imprimir UNA sola vez por delayInteligente o cuando cambie
        if (modoRemoto && serial_fd >= 0) {
            if (*delay_ms != ultimaVelocidadMostrada) {
                char msg[64];
                snprintf(msg, sizeof(msg),"\rDelay secuencia: %d ms - Velocidad secuencia: %.2f Hz   ", *delay_ms, 1000.0 / (double)(*delay_ms));
                serialPuts(serial_fd, msg);
                ultimaVelocidadMostrada = *delay_ms;
            }
        }

        if (manejarTeclado(orig_t, orig_flags, delay_ms))
            return 1; // salir

        delay(pasoSubDelay);
    }
    return 0;
}

// Ejecutar tablas de datos
static int ejecutarSecuenciaTabla(const unsigned char seq[][8], int n_frames, int *delayGuardado, int delayInicial) {
    struct termios orig_t;
    int orig_flags;

    if (setup_nocanonico_nobloq(&orig_t, &orig_flags) != 0)
        return 1;

    int delay_ms = (*delayGuardado > 0) ? *delayGuardado : delayInicial;

    while (1) {
        for (int i = 0; i < n_frames; i++) {
            aplicarEstado(seq[i]);

            if (delayInteligente(delay_ms, &orig_t, orig_flags, &delay_ms)) {
                *delayGuardado = delay_ms;
                return 0;
            }
        }
    }

    *delayGuardado = delay_ms;
    restaurarTerminal(&orig_t, orig_flags);
    apagarLeds();
    return 0;
}

// -------------------- Secuencia 1: Auto fantástico --------------------
int runAutoFantastico(int delayInicial) {
    struct termios orig_t;
    int orig_flags;

    if (setup_nocanonico_nobloq(&orig_t, &orig_flags) != 0)
        return 1;

    int delay_ms = (vel_auto > 0) ? vel_auto : delayInicial;
    int indice = 0, direccion = 1;

    while (1) {
        for (int j = 0; j < 8; j++)
            digitalWrite(LEDS[j], (j == indice) ? HIGH : LOW);

        if (delayInteligente(delay_ms, &orig_t, orig_flags, &delay_ms)) {
            vel_auto = delay_ms;
            return 0;
        }

        indice += direccion;
        if (indice == 7 || indice == 0)
            direccion = -direccion;
    }

    vel_auto = delay_ms;
    restaurarTerminal(&orig_t, orig_flags);
    apagarLeds();
    return 0;
}

// -------------------- Secuencia 2: El choque --------------------
int runChoque(int delayInicial) {
    return ejecutarSecuenciaTabla(choque, cantEstados_Choque, &vel_choque, delayInicial);
}

// -------------------- Secuencia 3: La apilada --------------------
int runApilada(int delayInicial) {
    struct termios orig_t;
    int orig_flags;

    if (setup_nocanonico_nobloq(&orig_t, &orig_flags) != 0)
        return 1;

    int delay_ms = (vel_apilada > 0) ? vel_apilada : delayInicial;
    unsigned char estado[8] = {0};
    int apilados = 0;

    while (apilados < 8) {
        int destino = 7 - apilados;

        for (int pos = 0; pos <= destino; pos++) {
            if (delayInteligente(delay_ms, &orig_t, orig_flags, &delay_ms)) {
                vel_apilada = delay_ms;
                return 0;
            }

            for (int j = 0; j < 8; j++) {
                if (estado[j])
                    digitalWrite(LEDS[j], HIGH);
                else if (j == pos)
                    digitalWrite(LEDS[j], HIGH);
                else
                    digitalWrite(LEDS[j], LOW);
            }
        }

        for (int k = 0; k < 4; k++) {
            if (delayInteligente(delay_ms / 2, &orig_t, orig_flags, &delay_ms)) {
                vel_apilada = delay_ms;
                return 0;
            }

            for (int j = 0; j < 8; j++) {
                if (estado[j])
                    digitalWrite(LEDS[j], HIGH);
                else if (j == destino)
                    digitalWrite(LEDS[j], (k % 2 == 0) ? HIGH : LOW);
                else
                    digitalWrite(LEDS[j], LOW);
            }
        }

        estado[destino] = 1;
        apilados++;
    }

    for (int j = 0; j < 8; j++)
        digitalWrite(LEDS[j], HIGH);
    delay(1000);

    vel_apilada = delay_ms;
    restaurarTerminal(&orig_t, orig_flags);
    apagarLeds();
    return 0;
}

// -------------------- Secuencia 4: La carrera --------------------
int runCarrera(int delayInicial) {
    return ejecutarSecuenciaTabla(carrera, cantEstados_Carrera, &vel_carrera, delayInicial);
}

// -------------------- Secuencia 5: Binario completo --------------------
int runBinarioCompleto(int delayInicial) {
    struct termios orig_t;
    int orig_flags;
    if (setup_nocanonico_nobloq(&orig_t, &orig_flags) != 0)
        return 1;

    int delay_ms = (vel_binario > 0) ? vel_binario : delayInicial;
    unsigned char frame[8];

    while (1) {
        for (unsigned int val = 0; val < 256; val++) {
            for (int j = 0; j < 8; j++)
                frame[j] = (val >> j) & 1;

            aplicarEstado(frame);

            if (delayInteligente(delay_ms, &orig_t, orig_flags, &delay_ms)) {
                vel_binario = delay_ms;
                return 0;
            }
        }
    }

    vel_binario = delay_ms;
    restaurarTerminal(&orig_t, orig_flags);
    apagarLeds();
    return 0;
}

// -------------------- Secuencia 6: Salto intermedio --------------------
int runDanza(int delayInicial) {
    return ejecutarSecuenciaTabla(danza, cantEstados_Danza, &vel_danza, delayInicial);
}

// -------------------- Secuencia 7: FOFO (First On, First Off) --------------------
int runFirstOnFirstOff(int delayInicial) {
    struct termios orig_t;
    int orig_flags;
    if (setup_nocanonico_nobloq(&orig_t, &orig_flags) != 0)
        return 1;

    int delay_ms = (vel_firstinfirstoff > 0) ? vel_firstinfirstoff : delayInicial;

    while (1) {
        for (int i = 0; i < 8; i++) { // Encendido progresivo
            // Enciende los LEDs desde el primero hasta el actual
            for (int j = 0; j <= i; j++)
                digitalWrite(LEDS[j], HIGH);
            // Asegura que los siguientes permanezcan apagados
            for (int j = i + 1; j < 8; j++)
                digitalWrite(LEDS[j], LOW);

            if (delayInteligente(delay_ms, &orig_t, orig_flags, &delay_ms)) {
                vel_firstinfirstoff = delay_ms;
                return 0;
            }
        }

        delay(delay_ms);

        for (int i = 0; i < 8; i++) { // Apagado progresivo
            // Apaga los LEDs desde el primero hasta el actual
            for (int j = 0; j <= i; j++)
                digitalWrite(LEDS[j], LOW);
            // Mantiene los siguientes encendidos hasta apagarlos después
            for (int j = i + 1; j < 8; j++)
                digitalWrite(LEDS[j], HIGH);

            if (delayInteligente(delay_ms, &orig_t, orig_flags, &delay_ms)) {
                vel_firstinfirstoff = delay_ms;
                return 0;
            }
        }
    }

    vel_firstinfirstoff = delay_ms;
    restaurarTerminal(&orig_t, orig_flags);
    apagarLeds();
    return 0;
}

// -------------------- Secuencia 8: Escalera central --------------------
int runEscaleraCentral(int delayInicial) {
    return ejecutarSecuenciaTabla(escaleraCentral, cantEstados_EscaleraCentral, &vel_escalera, delayInicial);
}

// -------------------- Reset de velocidades --------------------
void resetVelocidades(void) {
    vel_auto      = 0;
    vel_choque    = 0;
    vel_apilada   = 0;
    vel_carrera   = 0;
    vel_binario   = 0;
    vel_danza     = 0;
    vel_firstinfirstoff  = 0;
    vel_escalera  = 0;
}
