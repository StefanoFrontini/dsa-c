#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>    // Per open
#include <sys/mman.h> // Per mmap
#include <unistd.h>   // Per close, ftruncate

int main(void)
{
    const char *filepath = "database.txt";
    const char *messaggio = "Ciao! Sto scrivendo su disco fingendo che sia RAM.";
    size_t lunghezza = strlen(messaggio);

    // 1. Apriamo il file (Low Level I/O)
    // O_RDWR: Lettura e Scrittura
    // O_CREAT: Crea se non esiste
    // 0644: Permessi file (rw-r--r--)
    int fd = open(filepath, O_RDWR | O_CREAT, 0644);
    if (fd == -1)
    {
        perror("Errore open");
        exit(1);
    }

    // 2. Allarghiamo il file PRIMA di mapparlo
    // Mmap non può "inventare" spazio su disco. Il file deve essere grande abbastanza.
    // Lo allunghiamo esattamente quanto serve per il messaggio.
    if (ftruncate(fd, lunghezza) == -1)
    {
        perror("Errore ftruncate");
        exit(1);
    }

    // 3. LA MAGIA: Proiettiamo il file in RAM
    // NULL: Decidi tu OS dove metterlo
    // lunghezza: Quanti byte mappare
    // PROT_READ | PROT_WRITE: Voglio leggere e scrivere
    // MAP_SHARED: Le modifiche vanno scritte sul disco (Cruciale!)
    char *map = mmap(NULL, lunghezza, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (map == MAP_FAILED)
    {
        perror("Errore mmap");
        exit(1);
    }

    // Da questo momento, 'fd' non ci serve quasi più.
    // Abbiamo il puntatore 'map'.

    printf("File mappato all'indirizzo di memoria: %p\n", map);
    printf("Sto copiando i dati...\n");

    // 4. Scriviamo come se fosse un Array in memoria!
    // Non usiamo fwrite. Usiamo memcpy o assegnazione diretta.
    memcpy(map, messaggio, lunghezza);

    // Potremmo anche fare: map[0] = 'X';

    // 5. Sincronizzazione (Facoltativo ma consigliato)
    // Diciamo al Kernel: "Scrivi le pagine sporche su disco ORA".
    // Senza questo, il Kernel lo farebbe quando vuole lui.
    msync(map, lunghezza, MS_SYNC);

    // 6. Pulizia
    munmap(map, lunghezza); // Rimuovi la proiezione
    close(fd);              // Chiudi il file descriptor

    printf("Fatto. Controlla il file '%s'.\n", filepath);
    return 0;
}