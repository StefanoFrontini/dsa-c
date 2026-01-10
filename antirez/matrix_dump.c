
#include <stdio.h>
#include <string.h>

// Funzione che scarica la memoria su file
void dump_memory(char *filename, void *start_address, int length)
{
    // 1. Apriamo il file in modalità scrittura BINARIA ("wb")
    FILE *fp = fopen(filename, "wb");

    if (fp == NULL)
    {
        printf("Errore: Impossibile creare il file %s\n", filename);
        return;
    }

    // 2. Scriviamo i byte grezzi
    // fwrite(da_dove, dimensione_blocco, quanti_blocchi, su_che_file)
    fwrite(start_address, 1, length, fp);

    // 3. Chiudiamo il file
    fclose(fp);
    printf("\n[DUMP] Memoria salvata in '%s' (%d bytes)\n", filename, length);
    printf("       Da: %p\n", start_address);
    printf("       A:  %p\n", start_address + length);
}

int main(void)
{
    int is_admin = 0;
    char buffer[10];

    // Inizializzo buffer con delle 'B' per vederle bene nel file
    memset(buffer, 'B', 10);

    printf("Indirizzo buffer:   %p\n", (void *)&buffer);
    printf("Indirizzo is_admin: %p\n", (void *)&is_admin);

    // CALCOLIAMO QUANTA MEMORIA DUMPARE
    // Vogliamo prendere dal buffer fino alla fine di is_admin.
    // Calcoliamo la distanza tra i due puntatori + la grandezza dell'int
    long start_addr = (long)buffer;
    long end_addr = (long)&is_admin;

    // Nota: l'ordine in memoria può variare, calcoliamo la distanza assoluta
    // e aggiungiamo 4 byte per includere l'intero is_admin
    int lunghezza_dump = (int)(end_addr - start_addr) + sizeof(int);

    // Se per caso il compilatore ha messo is_admin PRIMA del buffer, correggiamo
    if (lunghezza_dump < 0)
        lunghezza_dump = 32; // Valore di default sicuro (dumpa 32 byte)

    // --- ESEGUIAMO IL DUMP ---
    dump_memory("memoria.bin", buffer, lunghezza_dump);
    // Ho messo 64 byte fissi per essere sicuro di prendere buffer, is_admin e anche il Return Address!

    return 0;
}