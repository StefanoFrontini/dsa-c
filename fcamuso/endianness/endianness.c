#include <stdio.h>
#include <stdint.h>

void stampa_byte_memoria(void *ptr, size_t size)
{
    unsigned char *byte_ptr = (unsigned char *)ptr;
    printf("Byte in memoria (dal primo all'ultimo indirizzo):\n");
    for (size_t i = 0; i < size; i++)
    {
        printf("  Indirizzo +%zu: 0x%02X\n", i, byte_ptr[i]);
    }
}

void verifica_endianness_metodo1()
{
    printf("=== METODO 1: Analisi diretta dei byte ===\n");

    int numero = 0x12345678; // Numero facilmente riconoscibile

    printf("Numero: 0x%08X\n", numero);
    printf("In binario: ");
    for (int k = 0; k < 32; k++)
    {
        if ((numero & (1U << (31 - k))) != 0)
        {
            printf("1");
        }
        else
        {
            printf("0");
        }
        if ((k + 1) % 8 == 0)
            printf(" ");
    }
    printf("\n\n");

    stampa_byte_memoria(&numero, sizeof(numero));

    unsigned char *ptr = (unsigned char *)&numero;

    if (ptr[0] == 0x78)
    {
        printf("\n✓ Sistema LITTLE ENDIAN\n");
        printf("  (il byte meno significativo 0x78 è al primo indirizzo)\n");
    }
    else if (ptr[0] == 0x12)
    {
        printf("\n✓ Sistema BIG ENDIAN\n");
        printf("  (il byte più significativo 0x12 è al primo indirizzo)\n");
    }
    printf("\n");
}

void verifica_endianness_metodo2()
{
    printf("=== METODO 2: Union trick ===\n");

    union
    {
        uint32_t intero;
        uint8_t byte[4];
    } test;

    test.intero = 0x01020304;

    printf("Intero: 0x%X\n", test.intero);
    printf("Byte array:\n");
    for (int i = 0; i < 4; i++)
    {
        printf("  byte[%d] = 0x%02X\n", i, test.byte[i]);
    }

    if (test.byte[0] == 0x04)
    {
        printf("\n✓ Sistema LITTLE ENDIAN\n");
    }
    else if (test.byte[0] == 0x01)
    {
        printf("\n✓ Sistema BIG ENDIAN\n");
    }
    printf("\n");
}

void verifica_endianness_metodo3()
{
    printf("=== METODO 3: Pointer casting ===\n");

    uint16_t valore = 0xABCD;
    uint8_t *ptr = (uint8_t *)&valore;

    printf("Valore: 0x%04X\n", valore);
    printf("Primo byte in memoria: 0x%02X\n", ptr[0]);
    printf("Secondo byte in memoria: 0x%02X\n", ptr[1]);

    if (ptr[0] == 0xCD)
    {
        printf("\n✓ Sistema LITTLE ENDIAN\n");
        printf("  (CD viene prima di AB in memoria)\n");
    }
    else if (ptr[0] == 0xAB)
    {
        printf("\n✓ Sistema BIG ENDIAN\n");
        printf("  (AB viene prima di CD in memoria)\n");
    }
    printf("\n");
}

void esempio_pratico()
{
    printf("=== ESEMPIO PRATICO: Perché l'endianness conta ===\n\n");

    int numero = -27; // 0xFFFFFFE5

    printf("Numero: %d (0x%08X)\n\n", numero, (unsigned int)numero);

    printf("Se salvi questo int su disco e lo leggi su una macchina\n");
    printf("con endianness diversa, ottieni un numero completamente diverso!\n\n");

    stampa_byte_memoria(&numero, sizeof(numero));

    printf("\nSe questi byte vengono letti in ordine inverso:\n");
    unsigned char *ptr = (unsigned char *)&numero;
    int numero_invertito = (ptr[3]) | (ptr[2] << 8) | (ptr[1] << 16) | (ptr[0] << 24);
    printf("Ottieni: %d (0x%08X)\n", numero_invertito, (unsigned int)numero_invertito);
}

int main(void)
{
    printf("╔═══════════════════════════════════════════╗\n");
    printf("║   VERIFICA ENDIANNESS DEL SISTEMA        ║\n");
    printf("╚═══════════════════════════════════════════╝\n\n");

    verifica_endianness_metodo1();
    printf("─────────────────────────────────────────────\n\n");

    verifica_endianness_metodo2();
    printf("─────────────────────────────────────────────\n\n");

    verifica_endianness_metodo3();
    printf("─────────────────────────────────────────────\n\n");

    esempio_pratico();

    return 0;
}

/*
 * SPIEGAZIONE COMPLETA:
 *
 * ENDIANNESS riguarda l'ORDINE DEI BYTE in memoria, non l'ordine dei bit.
 *
 * Per un int di 4 byte con valore 0x12345678:
 *
 * LITTLE ENDIAN (Intel x86/x64, ARM moderno):
 *   Indirizzo crescente →
 *   [78] [56] [34] [12]
 *   LSB              MSB
 *
 * BIG ENDIAN (network byte order, alcuni ARM, vecchi PowerPC):
 *   Indirizzo crescente →
 *   [12] [34] [56] [78]
 *   MSB              LSB
 *
 * IMPORTANTE:
 * - Il tuo programma originale stampa i BIT di un int, non i BYTE in memoria
 * - L'endianness non influenza la rappresentazione dei bit all'interno di un byte
 * - L'endianness conta quando:
 *   * Leggi/scrivi dati multi-byte da/su disco
 *   * Trasmetti dati in rete
 *   * Condividi dati tra sistemi con endianness diversa
 *   * Fai type punning (cast tra tipi diversi via puntatori)
 */