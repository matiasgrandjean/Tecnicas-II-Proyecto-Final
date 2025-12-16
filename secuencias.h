#ifndef SECUENCIAS_H
#define SECUENCIAS_H

extern const unsigned char LEDS[8];

int runAutoFantastico(int delayInicial);
int runChoque(int delayInicial);
int runApilada(int delayInicial);
int runCarrera(int delayInicial);
int runBinarioCompleto(int delayInicial);
int runDanza(int delayInicial);
int runFirstOnFirstOff(int delayInicial);
int runEscaleraCentral(int delayInicial);

void resetVelocidades(void);

#endif
