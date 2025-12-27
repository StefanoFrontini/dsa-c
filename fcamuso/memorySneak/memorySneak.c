#include <stdio.h>
#include <stdint.h>

void stampa_binario(int num)
{
    printf("Rappresentazione binaria di %d:\n", num);

    for (int k = 0; k < 32; k++)
    {
        // Uso 1U (unsigned) per evitare undefined behavior
        if ((num & (1U << (31 - k))) != 0)
        {
            printf("1");
        }
        else
        {
            printf("0");
        }

        // Aggiungo uno spazio ogni 8 bit per leggibilità
        if ((k + 1) % 8 == 0)
        {
            printf(" ");
        }
    }
    printf("\n\n");
}

int main(void)
{
    int numero = -27;

    printf("=== VERIFICA COMPLEMENTO A 2 ===\n\n");

    // Stampa il numero negativo
    stampa_binario(numero);

    // Verifica manuale del complemento a 2 di -27
    printf("Verifica manuale:\n");
    printf("27 in binario:     00000000 00000000 00000000 00011011\n");
    printf("Inverti i bit:     11111111 11111111 11111111 11100100\n");
    printf("Aggiungi 1:        11111111 11111111 11111111 11100101\n");
    printf("                   (questo è -27 in complemento a 2)\n\n");

    // Altri esempi
    printf("Altri esempi:\n");
    stampa_binario(-1);
    stampa_binario(-128);
    stampa_binario(27); // Versione positiva per confronto

    // Verifica interessante: -1 è tutti bit a 1
    printf("Nota: -1 in complemento a 2 è rappresentato con tutti i bit a 1!\n");

    return 0;
}

/*
 * SPIEGAZIONE:
 *
 * Il complemento a 2 è il metodo usato per rappresentare numeri negativi:
 * 1. Prendi il valore assoluto del numero in binario
 * 2. Inverti tutti i bit (0→1, 1→0)
 * 3. Aggiungi 1
 *
 * Per -27:
 * - 27 in binario (32 bit): 00000000 00000000 00000000 00011011
 * - Inverti:                11111111 11111111 11111111 11100100
 * - Aggiungi 1:             11111111 11111111 11111111 11100101
 *
 * CORREZIONI AL TUO CODICE:
 * 1. Uso 1U invece di 1L per evitare problemi con la dimensione di long
 * 2. Confronto con != 0 invece di > 0 (più chiaro e corretto)
 * 3. Aggiunti spazi ogni 8 bit per leggibilità
 */