#include <wiringPi.h>
#include <wiringSerial.h>
#include <pcf8591.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

#include "nocanonico.h"
#include "secuencias.h"

#define BASE 120
#define ADDR 0x48
#define FD_STDIN 0
#define CLAVE_CORRECTA "renzo123"

#define UART "/dev/ttyAMA0"
#define BAUDRATE 38400

extern int map(int x, int in_min, int in_max, int out_min, int out_max);

int autenticar();
int ajustar_velocidad_inicial(int delay_actual);


int serial_fd = -1;     // descriptor UART (se usa en modo remoto)
int modoRemoto = 0;    // 0 = local, 1 = remoto

int main() {
    int opcion;
    int modo = 0;           // 1 = local, 2 = remoto
    int modo_forzado = 0;   // para cambiar de modo desde la opción 12

    system("clear");

    // Iniciar GPIO
    if (wiringPiSetupGpio() == -1) {
        fprintf(stderr, "Error al inicializar wiringPi\n");
        return 1;
    }

    // LEDs como salida y en "LOW"
    for (int i = 0; i < 8; i++) {
        pinMode(LEDS[i], OUTPUT);
        digitalWrite(LEDS[i], LOW);
    }

    pcf8591Setup(BASE, ADDR);
    
    // Iniciar sesión
    if (!autenticar()) {
        return 1;
    }

    // Leer velocidad inicial desde el ADC
    int val_adc    = analogRead(BASE + 0);
    int delay_inicial = map(val_adc, 0, 255, 50, 2000); // Función externa map desde map.s

    // void loop()
    while (1) {

        // ---------------- MODO DE TRABAJO: LOCAL O REMOTO ----------------
        if (modo_forzado != 0) {
            // Se pidió cambio directo de modo desde el menú
            modo = modo_forzado;
            modo_forzado = 0;
        } else {
            // Menú normal de selección de modo
            do {
                system("clear");
                printf("Seleccione modo de trabajo:\n");
                printf("1. LOCAL  (teclado de la Raspberry)\n");
                printf("2. REMOTO (PC via Arduino + UART TTL)\n\n");
                printf("Opcion: ");

                char buf_modo[16];
                if (!fgets(buf_modo, sizeof(buf_modo), stdin)) {
                    clearerr(stdin);
                    modo = 0;
                } else {
                    if (sscanf(buf_modo, "%d", &modo) != 1)
                        modo = 0;
                }

            } while (modo != 1 && modo != 2);
        }

        // Configurar según modo elegido / forzado
        if (modo == 2) {
            // ------ MODO REMOTO ------
            modoRemoto = 1;

            // Inicializar UART
            if (serial_fd < 0) {
                int fd = serialOpen(UART, BAUDRATE);
                if (fd < 0) {
                    fprintf(stderr, "Error al abrir /dev/ttyAMA0 en modo remoto\n");
                    modoRemoto = 0;
                } else {
                    serial_fd = fd;
                }
            }

            system("clear");
            printf("Ejecutando modo remoto...\n");

            if (serial_fd >= 0) {
                serialPuts(serial_fd, "\033[2J\033[H");
                serialPuts(serial_fd, "Ejecutando modo remoto...\r\n");
            }

        } else {
            // ------ MODO LOCAL ------
            modoRemoto = 0;

            if (serial_fd < 0) {
                int fd = serialOpen(UART, BAUDRATE);
                if (fd >= 0) {
                    serial_fd = fd;
                }
            }

            system("clear");
            printf("Ejecutando modo local...\n");

            if (serial_fd >= 0) {
                serialPuts(serial_fd, "\033[2J\033[H");
                serialPuts(serial_fd, "Ejecutando modo local...\r\n");
            }
        }

        // ------------------------ MODO LOCAL ------------------------
        if (!modoRemoto) {
            int volver_a_modos = 0;

            while (!volver_a_modos) {

                do {
                    system("clear");
                    printf("Menu principal del proyecto final (secuencias de luces) [LOCAL]\n");
                    printf("1. El auto fantástico\n");
                    printf("2. El choque\n");
                    printf("3. La apilada\n");
                    printf("4. La carrera\n");
                    printf("5. Contador binario completo\n");
                    printf("6. Danza de luces\n");
                    printf("7. First On - First Off\n");
                    printf("8. Escalera central\n");
                    printf("9. Ajustar velocidad inicial de las secuencias\n");
                    printf("10. Resetear velocidades de las secuencias\n");
                    printf("11. Salir\n");
                    printf("12. Cambiar al modo remoto\n\n");
                    printf("Delay inicial = %d ms - Velocidad inicial = %.2f Hz\n", delay_inicial, 1000.0 / (double)(delay_inicial));
                    printf("Seleccione una opcion: ");

                    char buffer[32];
                    if (!fgets(buffer, sizeof(buffer), stdin)) {
                        clearerr(stdin);
                        opcion = 0;
                    } else {
                        if (sscanf(buffer, "%d", &opcion) != 1)
                            opcion = 0;
                    }

                } while(opcion <= 0 || opcion > 12);
                
                switch(opcion){
                case 1:
                    system("clear");
                    printf("Ejecutando secuencia 'El auto fantástico'\n");   
                    printf("Presione 'q' para salir, flechas ↑/↓ para velocidad.\n"); 
                    runAutoFantastico(delay_inicial);
                    break;

                case 2: 
                    system("clear");
                    printf("Ejecutando secuencia 'El choque'\n");   
                    printf("Presione 'q' para salir, flechas ↑/↓ para velocidad.\n"); 
                    runChoque(delay_inicial);
                    break;

                case 3:
                    system("clear");
                    printf("Ejecutando secuencia 'La apilada'\n");   
                    printf("Presione 'q' para salir, flechas ↑/↓ para velocidad.\n"); 
                    runApilada(delay_inicial);
                    break;     

                case 4:
                    system("clear");
                    printf("Ejecutando secuencia 'La carrera'\n");   
                    printf("Presione 'q' para salir, flechas ↑/↓ para velocidad.\n"); 
                    runCarrera(delay_inicial);
                    break;

                case 5:
                    system("clear");
                    printf("Ejecutando secuencia 'Contador binario completo'\n");
                    printf("Presione 'q' para salir, flechas ↑/↓ para velocidad.\n");
                    runBinarioCompleto(delay_inicial);
                    break;

                case 6:
                    system("clear");
                    printf("Ejecutando secuencia 'Danza de luces'\n");
                    printf("Presione 'q' para salir, flechas ↑/↓ para velocidad.\n");
                    runDanza(delay_inicial);
                    break;

                case 7:
                    system("clear");
                    printf("Ejecutando secuencia 'First On - First Off'\n");
                    printf("Presione 'q' para salir, flechas ↑/↓ para velocidad.\n");
                    runFirstOnFirstOff(delay_inicial);
                    break;

                case 8:
                    system("clear");
                    printf("Ejecutando secuencia 'Escalera central'\n");
                    printf("Presione 'q' para salir, flechas ↑/↓ para velocidad.\n");
                    runEscaleraCentral(delay_inicial);
                    break;

                case 9:
                    delay_inicial = ajustar_velocidad_inicial(delay_inicial);
                    break;

                case 10:
                    resetVelocidades();
                    system("clear");
                    printf("Velocidades de las secuencias reseteadas.\n");
                    printf("Ahora comenzarán nuevamente con un retardo inicial de (%d ms).\n", delay_inicial);
                    printf("Presione ENTER para volver al menu...\n");
                    getchar();
                    break;

                case 11:
                    system("clear");
                    printf("Saliendo del programa...\n");
                    return 0;

                case 12:
                    // Cambiar directamente al modo REMOTO
                    modo_forzado = 2;
                    volver_a_modos = 1;
                    break;

                default:
                    break;
                }
            }
        } 

        // ------------------------ MODO REMOTO ------------------------
        else {
            int volver_a_modos = 0;

            while (!volver_a_modos) {

                if (serial_fd >= 0) {
                    serialPuts(serial_fd, "\033[2J\033[H");

                    // Enviar menú por UART al PC
                    serialPuts(serial_fd, "Menu principal del proyecto final (secuencias de luces) [REMOTO]\r\n");
                    serialPuts(serial_fd, "1. El auto fantastico\r\n");
                    serialPuts(serial_fd, "2. El choque\r\n");
                    serialPuts(serial_fd, "3. La apilada\r\n");
                    serialPuts(serial_fd, "4. La carrera\r\n");
                    serialPuts(serial_fd, "5. Contador binario completo\r\n");
                    serialPuts(serial_fd, "6. Danza de luces\r\n");
                    serialPuts(serial_fd, "7. First On - First Off\r\n");
                    serialPuts(serial_fd, "8. Escalera central\r\n");
                    serialPuts(serial_fd, "9. Ajustar velocidad inicial de las secuencias\r\n");
                    serialPuts(serial_fd, "10. Resetear velocidades de las secuencias\r\n");
                    serialPuts(serial_fd, "11. Salir\r\n");
                    serialPuts(serial_fd, "12. Cambiar al modo local\r\n\r\n");

                    char linea[80];
                    snprintf(linea, sizeof(linea),
                             "Velocidad inicial = %d ms\r\n", delay_inicial);
                    serialPuts(serial_fd, linea);

                    serialPuts(serial_fd, "Seleccione una opcion: ");
                }

                // Leer una línea desde el PC
                char buf[16];
                int idx = 0;
                opcion = 0;

                memset(buf, 0, sizeof(buf));

                while (1) {
                    if (serial_fd >= 0 && serialDataAvail(serial_fd)) {
                        unsigned char c = (unsigned char)serialGetchar(serial_fd);
            
                        if (c == 127 || c == 8) {
                            if (idx > 0) {
                                idx--;
                                buf[idx] = '\0';
                                serialPuts(serial_fd, "\b \b");
                            }
                            continue;
                        }

                        if (c == '\r' || c == '\n') {
                            buf[idx] = '\0';
                            serialPuts(serial_fd, "\r\n");
                            break;
                        }

                        // Guardar caracteres normales
                        if (idx < (int)sizeof(buf) - 1) {
                            buf[idx++] = c;
                            serialPutchar(serial_fd, c);  // Eco del carácter
                        }
                    }
                    delay(10);
                }

                if (sscanf(buf, "%d", &opcion) != 1) {
                    opcion = 0;
                }

                switch (opcion) {
                case 1:
                    serialPuts(serial_fd, "\033[2J\033[H");
                    serialPuts(serial_fd, "Ejecutando secuencia 'El auto fantastico'\r\n");
                    serialPuts(serial_fd, "Presione 'q' para salir, flechas ↑/↓ para velocidad.\r\n");
                    runAutoFantastico(delay_inicial);
                    break;

                case 2:
                    serialPuts(serial_fd, "\033[2J\033[H");
                    serialPuts(serial_fd, "Ejecutando secuencia 'El choque'\r\n");
                    serialPuts(serial_fd, "Presione 'q' para salir, flechas ↑/↓ para velocidad.\r\n");
                    runChoque(delay_inicial);
                    break;

                case 3:
                    serialPuts(serial_fd, "\033[2J\033[H");
                    serialPuts(serial_fd, "Ejecutando secuencia 'La apilada'\r\n");
                    serialPuts(serial_fd, "Presione 'q' para salir, flechas ↑/↓ para velocidad.\r\n");
                    runApilada(delay_inicial);
                    break;

                case 4:
                    serialPuts(serial_fd, "\033[2J\033[H");
                    serialPuts(serial_fd, "Ejecutando secuencia 'La carrera'\r\n");
                    serialPuts(serial_fd, "Presione 'q' para salir, flechas ↑/↓ para velocidad.\r\n");
                    runCarrera(delay_inicial);
                    break;

                case 5:
                    serialPuts(serial_fd, "\033[2J\033[H");
                    serialPuts(serial_fd, "Ejecutando secuencia 'Contador binario completo'\r\n");
                    serialPuts(serial_fd, "Presione 'q' para salir, flechas ↑/↓ para velocidad.\r\n");
                    runBinarioCompleto(delay_inicial);
                    break;

                case 6:
                    serialPuts(serial_fd, "\033[2J\033[H");
                    serialPuts(serial_fd, "Ejecutando secuencia 'Danza de luces'\r\n");
                    serialPuts(serial_fd, "Presione 'q' para salir, flechas ↑/↓ para velocidad.\r\n");
                    runDanza(delay_inicial);
                    break;

                case 7:
                    serialPuts(serial_fd, "\033[2J\033[H");
                    serialPuts(serial_fd, "Ejecutando secuencia 'First On - First Off'\r\n");
                    serialPuts(serial_fd, "Presione 'q' para salir, flechas ↑/↓ para velocidad.\r\n");
                    runFirstOnFirstOff(delay_inicial);
                    break;

                case 8:
                    serialPuts(serial_fd, "\033[2J\033[H");
                    serialPuts(serial_fd, "Ejecutando secuencia 'Escalera central'\r\n");
                    serialPuts(serial_fd, "Presione 'q' para salir, flechas ↑/↓ para velocidad.\r\n");
                    runEscaleraCentral(delay_inicial);
                    break;

                case 9: {
                    int nuevo_delay = delay_inicial;

                    serialPuts(serial_fd, "\033[2J\033[H");
                    serialPuts(serial_fd, "AJUSTE DE VELOCIDAD INICIAL (REMOTO)\r\n");
                    serialPuts(serial_fd, "-----------------------------------\r\n");
                    serialPuts(serial_fd, "Gire el potenciometro para cambiar la velocidad.\r\n");
                    serialPuts(serial_fd, "Presione ENTER para confirmar y volver al menu.\r\n\r\n");
                    serialPuts(serial_fd, "Lectura ADC actual y velocidad:\r\n");

                    while (1) {
                        int val_adc_r = analogRead(BASE + 0);
                        nuevo_delay = map(val_adc_r, 0, 255, 50, 2000);

                        char msg[200];
                        snprintf(msg, sizeof(msg), "ADC: %4d   Velocidad inicial medida: %4d ms   \r", val_adc_r, nuevo_delay);
                        serialPuts(serial_fd, msg);

                        // Ventana de tiempo corta para ver ENTER sin frenar todo
                        int confirmado = 0;
                        unsigned long t0 = millis();
                        while (millis() - t0 < 150) { // ~150 ms
                            if (serialDataAvail(serial_fd)) {
                                char c = (char)serialGetchar(serial_fd);
                                if (c == '\r' || c == '\n') {
                                    confirmado = 1;
                                    break;
                                }
                            }
                            delay(5);
                        }
                        if (confirmado) {
                            delay_inicial = nuevo_delay;
                            serialPuts(serial_fd, "\r\n");
                            break;
                        }
                    }
                    break;
                }

                case 10:
                    resetVelocidades();
                    serialPuts(serial_fd, "\033[2J\033[H");
                    {
                        char aux[160];
                        snprintf(aux, sizeof(aux),
                            "Velocidades de las secuencias reseteadas.\r\n"
                            "Ahora comenzaran nuevamente con un retardo inicial de (%d ms).\r\n"
                            "Presione ENTER para volver al menu...\r\n",
                            delay_inicial);
                        serialPuts(serial_fd, aux);
                    }
                    while (1) {
                        if (serialDataAvail(serial_fd)) {
                            char c = (char)serialGetchar(serial_fd);
                            if (c == '\r' || c == '\n') break;
                        }
                        delay(10);
                    }
                    break;

                case 11:
                    serialPuts(serial_fd, "\033[2J\033[H");
                    serialPuts(serial_fd, "Saliendo del programa (modo remoto)...\r\n");
                    system("clear");
                    printf("Saliendo del programa...\n");
                    return 0;

                case 12:
                    // Cambiar al modo LOCAL
                    serialPuts(serial_fd, "\033[2J\033[H");
                    serialPuts(serial_fd, "Cambiando al modo local...\r\n");
                    modo_forzado = 1;
                    volver_a_modos = 1;
                    break;

                default:
                    serialPuts(serial_fd, "\r\nOpcion invalida.\r\n");
                    break;
                }
            }

        }
    }
    return 0;
}

// -------------------- Función para autenticar al usuario --------------------
int autenticar() {
    const char clave_correcta[] = CLAVE_CORRECTA;
    int intentos = 0;
    
    while (intentos < 3) {
        char tec;
        struct termios t_old;
        
        // Activar modo no canónico bloqueante sin eco
        if (setup_nocanonico_bloq(&t_old) != 0) {
            printf("Error configurando la terminal.\n");
            return 0;
        }
        
        char password[256];
        int len = 0;
        tec = 0;

        printf("Ingrese contraseña: ");
        while (tec != 10 && len < (int)sizeof(password) - 1) {
            tec = getchar();
            if (tec == 127 || tec == 8) {
                if (len > 0) {
                    len--;
                    printf("\b \b");
                }
            } else if (tec != 10) {
                password[len++] = tec;
                printf("*");
            }
            fflush(stdout);
        }
        password[len] = '\0';

        // Restaurar terminal
        restaurarTerminal(&t_old, -1);

        if (strcmp(password, clave_correcta) == 0) {
            printf("\nAcceso concedido.\n\n");
            return 1;
        } else {
            intentos++;
            printf("\nContraseña incorrecta. Intento %d de 3.\n", intentos);
            if (intentos == 3) {
                printf("Demasiados intentos. Cerrando programa.\n");
                return 0;
            }
        }
    }
    return 0;
}

// Ajuste de velocidad inicial
int ajustar_velocidad_inicial(int delay_actual) {
    struct termios orig_t;
    int orig_flags;

    // Modo no canónico no bloqueante para leer ENTER
    if (setup_nocanonico_nobloq(&orig_t, &orig_flags) != 0) {
        printf("Error configurando terminal para ajuste de velocidad.\n");
        return delay_actual;
    }

    int nuevo_delay = delay_actual;

    while (1) {
        int val_adc = analogRead(BASE + 0);
        nuevo_delay = map(val_adc, 0, 255, 50, 2000);

        system("clear");
        printf("AJUSTE DE VELOCIDAD INICIAL\n");
        printf("----------------------------\n");
        printf("Gire el potenciómetro para cambiar la velocidad.\n");
        printf("Presione ENTER para confirmar y volver al menú.\n\n");
        printf("Lectura ADC actual : %d\n", val_adc);
        printf("Delay inicial medido: %d ms\n", nuevo_delay);
        printf("Velocidad inicial medida: %.2f Hz\n", 1000.0 / (double)(nuevo_delay));

        // Ver si se presionó ENTER
        char c;
        ssize_t n = read(STDIN_FILENO, &c, 1);
        if (n == 1 && (c == '\n' || c == '\r')) {
            break; // Confirmar velocidad y salir
        }

        // Pequeño delay de refresco (100 ms)
        usleep(100000);
    }

    // Restaurar terminal
    restaurarTerminal(&orig_t, orig_flags);

    return nuevo_delay;
}
