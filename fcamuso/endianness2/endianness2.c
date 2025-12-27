#include <stdio.h>
#include <stdint.h>

int main(void)
{
    printf("╔═══════════════════════════════════════════════════════╗\n");
    printf("║  DIFFERENZA TRA ACCESSO AI BYTE E OPERAZIONI BIT     ║\n");
    printf("╚═══════════════════════════════════════════════════════╝\n\n");

    // La variabile è SEMPRE in RAM (nello stack)
    int i = 0x12345678;

    printf("Variabile: int i = 0x12345678\n");
    printf("Indirizzo in RAM: %p\n\n", (void *)&i);

    // ═══════════════════════════════════════════════════════
    // METODO 1: Accesso ai BYTE individuali (dipende dall'endianness)
    // ═══════════════════════════════════════════════════════
    printf("═══ METODO 1: Accesso ai BYTE via puntatore ═══\n");
    printf("(questo DIPENDE dall'endianness)\n\n");

    unsigned char *byte_ptr = (unsigned char *)&i;

    printf("Byte in RAM (in ordine di indirizzo crescente):\n");
    for (int k = 0; k < 4; k++)
    {
        printf("  byte_ptr[%d] = 0x%02X  (indirizzo: %p)\n",
               k, byte_ptr[k], (void *)&byte_ptr[k]);
    }

    // Determina endianness
    printf("\nSu questo sistema: ");
    if (byte_ptr[0] == 0x78)
    {
        printf("LITTLE ENDIAN\n");
        printf("  → Byte meno significativo (0x78) all'indirizzo più basso\n");
    }
    else if (byte_ptr[0] == 0x12)
    {
        printf("BIG ENDIAN\n");
        printf("  → Byte più significativo (0x12) all'indirizzo più basso\n");
    }

    printf("\n");

    // ═══════════════════════════════════════════════════════
    // METODO 2: Operazioni bit a bit (NON dipende dall'endianness)
    // ═══════════════════════════════════════════════════════
    printf("═══ METODO 2: Operazioni BIT A BIT ═══\n");
    printf("(questo NON dipende dall'endianness)\n\n");

    printf("Rappresentazione binaria completa:\n  ");
    for (int k = 0; k < 32; k++)
    {
        if ((i & (1U << (31 - k))) != 0)
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

    printf("Test di alcuni bit specifici:\n");
    printf("  bit 0 (LSB):  %d\n", (i & (1U << 0)) ? 1 : 0);
    printf("  bit 7:        %d\n", (i & (1U << 7)) ? 1 : 0);
    printf("  bit 16:       %d\n", (i & (1U << 16)) ? 1 : 0);
    printf("  bit 31 (MSB): %d\n", (i & (1U << 31)) ? 1 : 0);

    printf("\n");

    // ═══════════════════════════════════════════════════════
    // CONFRONTO DIRETTO
    // ═══════════════════════════════════════════════════════
    printf("═══ CONFRONTO DIRETTO ═══\n\n");

    printf("Primo byte via puntatore: 0x%02X\n", byte_ptr[0]);
    printf("  Su little endian: 0x78 (byte meno significativo)\n");
    printf("  Su big endian:    0x12 (byte più significativo)\n");
    printf("  → DIPENDE dall'endianness!\n\n");

    printf("Byte meno significativo via bit operations:\n");
    printf("  (i & 0xFF) = 0x%02X\n", i & 0xFF);
    printf("  Su little endian: 0x78\n");
    printf("  Su big endian:    0x78\n");
    printf("  → NON dipende dall'endianness!\n\n");

    // ═══════════════════════════════════════════════════════
    // SPIEGAZIONE DEL PERCHÉ
    // ═══════════════════════════════════════════════════════
    printf("═══ SPIEGAZIONE ═══\n\n");

    printf("ACCESSO AI BYTE (byte_ptr[0]):\n");
    printf("  → Leggi direttamente dalla RAM\n");
    printf("  → Vedi i byte nell'ordine fisico in cui sono memorizzati\n");
    printf("  → L'ordine dipende dall'endianness\n\n");

    printf("OPERAZIONI BIT A BIT (i & 0xFF):\n");
    printf("  → Il processore carica 'i' dalla RAM in un registro\n");
    printf("  → Durante il caricamento, ricostruisce il valore corretto\n");
    printf("     (su little endian: legge [78 56 34 12] → 0x12345678)\n");
    printf("     (su big endian: legge [12 34 56 78] → 0x12345678)\n");
    printf("  → L'operazione & 0xFF estrae sempre gli 8 bit meno significativi\n");
    printf("  → Risultato: sempre 0x78, indipendentemente dall'endianness\n\n");

    // ═══════════════════════════════════════════════════════
    // CASO PRATICO: Quando l'endianness causa problemi
    // ═══════════════════════════════════════════════════════
    printf("═══ QUANDO L'ENDIANNESS CAUSA PROBLEMI ═══\n\n");

    printf("Scenario: Scrivi 'i' su un file byte per byte\n\n");

    printf("FILE file = fopen(\"data.bin\", \"wb\");\n");
    printf("fwrite(&i, sizeof(int), 1, file);  // Scrive i 4 byte in memoria\n");
    printf("fclose(file);\n\n");

    printf("Su little endian, il file conterrà: 78 56 34 12\n");
    printf("Su big endian, il file conterrà:    12 34 56 78\n\n");

    printf("Se leggi questo file su una macchina con endianness DIVERSA,\n");
    printf("otterrai un valore SBAGLIATO!\n\n");

    printf("Soluzione: usa funzioni di conversione di rete (htonl/ntohl)\n");
    printf("o salva i byte esplicitamente in un ordine definito.\n\n");

    // ═══════════════════════════════════════════════════════
    // DIMOSTRAZIONE FINALE
    // ═══════════════════════════════════════════════════════
    printf("═══ DIMOSTRAZIONE FINALE ═══\n\n");

    printf("Ricostruisci 'i' dai byte individuali:\n\n");

    // Metodo 1: Accesso diretto (dipende dall'endianness)
    int ricostruito_diretto = *((int *)byte_ptr);
    printf("1. Cast diretto:         0x%08X %s\n",
           ricostruito_diretto,
           (ricostruito_diretto == i) ? "✓" : "✗");

    // Metodo 2: Ricostruzione manuale little endian
    int ricostruito_le = byte_ptr[0] | (byte_ptr[1] << 8) |
                         (byte_ptr[2] << 16) | (byte_ptr[3] << 24);
    printf("2. Little endian manual: 0x%08X %s\n",
           ricostruito_le,
           (ricostruito_le == i) ? "✓" : "✗");

    // Metodo 3: Ricostruzione manuale big endian
    int ricostruito_be = (byte_ptr[0] << 24) | (byte_ptr[1] << 16) |
                         (byte_ptr[2] << 8) | byte_ptr[3];
    printf("3. Big endian manual:    0x%08X %s\n",
           ricostruito_be,
           (ricostruito_be == i) ? "✓" : "✗");

    printf("\nSu questo sistema, il metodo che funziona indica l'endianness!\n");

    return 0;
}

/*
 * RIEPILOGO COMPLETO:
 *
 * 1. LA VARIABILE È SEMPRE IN RAM
 *    - int i; → allocata nello stack (che è in RAM)
 *    - static int i; → allocata nel segmento dati (che è in RAM)
 *    - int *p = malloc(...); → allocata nell'heap (che è in RAM)
 *    - Creare un puntatore NON cambia dove è allocata!
 *
 * 2. ACCESSO AI BYTE (via puntatore)
 *    - Leggi direttamente dalla memoria fisica
 *    - Vedi i byte nell'ordine in cui sono disposti in RAM
 *    - DIPENDE dall'endianness
 *
 * 3. OPERAZIONI BIT A BIT
 *    - Il processore carica il valore in un registro
 *    - Compensa automaticamente per l'endianness durante il caricamento
 *    - Le operazioni vedono sempre il valore logico corretto
 *    - NON dipende dall'endianness
 *
 * 4. QUANDO L'ENDIANNESS CONTA
 *    - Quando accedi ai byte individuali via puntatori
 *    - Quando scrivi/leggi file binari
 *    - Quando trasmetti dati in rete
 *    - Quando fai type punning (cast tra tipi diversi)
 *
 * 5. IL TUO PROGRAMMA ORIGINALE
 *    - Usa operazioni bit a bit (i & (1L << k))
 *    - NON dipende dall'endianness
 *    - Produce lo stesso output su little e big endian
 *    - Mostra sempre la rappresentazione corretta del valore
 */