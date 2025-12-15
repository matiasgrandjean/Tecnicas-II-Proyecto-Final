// nocanonico.c

// Estado global para poder restaurar en el handler de Ctrl-C
static struct termios g_orig_t;
static int g_orig_flags = -1;

// Handler para Ctrl-C: restaura terminal y sale
static void restaurar_y_salir(int sig) {
    (void)sig;
    tcsetattr(STDIN_FILENO, TCSANOW, &g_orig_t);
    if (g_orig_flags != -1) {
        fcntl(STDIN_FILENO, F_SETFL, g_orig_flags);
    }
    _exit(0);
}

// ---- Modo no canónico BLOQUEANTE (para contraseña, usa getchar()) ----
// Devuelve 0 si OK, 1 si error.
// Guarda la configuración original en *orig_t.
// NO toca VMIN/VTIME, así que getchar() sigue bloqueando.
int setup_nocanonico_bloq(struct termios *orig_t) {
    struct termios t;

    if (tcgetattr(STDIN_FILENO, &t) == -1) {
        return 1;
    }

    if (orig_t) *orig_t = t;
    g_orig_t = t;

    // No canónico y sin eco (como tenías en autenticar)
    t.c_lflag &= ~(ECHO | ICANON);

    if (tcsetattr(STDIN_FILENO, TCSANOW, &t) == -1) {
        return 1;
    }

    g_orig_flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    signal(SIGINT, restaurar_y_salir);

    return 0;
}

// ---- Modo no canónico NO BLOQUEANTE (para secuencias con read() no bloqueante) ----
// Devuelve 0 si OK, 1 si error.
// Guarda configuración original en *orig_t y flags originales en *orig_flags.
int setup_nocanonico_nobloq(struct termios *orig_t, int *orig_flags) {
    struct termios t;

    if (tcgetattr(STDIN_FILENO, &t) == -1) {
        return 1;
    }

    if (orig_t) *orig_t = t;
    g_orig_t = t;

    t.c_lflag &= ~(ECHO | ICANON);
    t.c_cc[VMIN]  = 0;
    t.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &t) == -1) {
        tcsetattr(STDIN_FILENO, TCSANOW, &g_orig_t);
        return 1;
    }

    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (flags == -1) {
        tcsetattr(STDIN_FILENO, TCSANOW, &g_orig_t);
        return 1;
    }

    if (orig_flags) *orig_flags = flags;
    g_orig_flags = flags;

    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    signal(SIGINT, restaurar_y_salir);

    return 0;
}

// ---- Restaurar terminal ----
// Si orig_flags == -1, no toca flags.
void restaurarTerminal(const struct termios *orig_t, int orig_flags) {
    if (orig_t) {
        tcsetattr(STDIN_FILENO, TCSANOW, orig_t);
    }
    if (orig_flags != -1) {
        fcntl(STDIN_FILENO, F_SETFL, orig_flags);
    }
}
