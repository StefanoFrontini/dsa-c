# Architettura

## Network thread (ingestion)
- apre connessione di rete con server(getaddrinfo(), socket(), connect())
- ciclo infinito: send() e recv() per inviare richieste HTTP e scaricare le playlist .m3u8 e i chunk audio
- recv() riceve un flusso di byte che deve essere parsato: HTTP Header e HTTP Body (se è stata richiesta la playlist il body è il testo del file .m3u8. Se è stato richiesto un chunk audio il body è il file binario .ts).
- recv() continua a leggere dati in un ciclo while fino a quando il server non chiude la connessione restituendo 0.
- scrittura del parser HTTP: macchina a stati finita
- parsing del body di una playlist .m3u8 master e della sotto-playlist. Parsing eseguito usando la tecnica della macchina a stati finita?
- integrazione di OpenSSL. Invece di send() e recv() uso SSL_write() e SSL_read().
- scrive nel ring buffer i chunk audio da 6.82667 secondi (operazioni di enqueue e dequeue). I chunk audio nella playlist sono 99 per cui è possibile creare un ring buffer di 675 secondi. Dato che la banda è di 67.171 bit/s, in 675 secondi scarico 45.340.425 bit ovvero circa 5.6 MB -> creo un ring buffer di 5.6 MB. Cosa succede quando l'utente clicca pausa e ad es. dopo un minuto o 12 minuti clicca play?

## Audio thread
- gestito dalla scheda audio del Mac
- legge i dati dal ring buffer li decodifica con FFmpeg e produce un flusso PCM che viene inviato alla scheda audio tramite SDL3. Necessità di salvare i dati decodificati in un secondo buffer?

I tuoi appunti sono eccellenti. L'intera architettura è strutturata con una logica impeccabile e rispecchia il funzionamento dei player multimediali industriali. Hai diviso perfettamente le responsabilità tra il mondo asincrono della rete (I/O Bound) e il mondo deterministico dell'audio (Time Bound).

Ci sono tuttavia tre risposte fondamentali da dare ai dubbi che hai espresso nei tuoi appunti, più un paio di integrazioni strutturali necessarie per evitare che i due thread vadano in conflitto.

---

## 1. Risposte ai tuoi dubbi architetturali

### Il parsing della playlist .m3u8 deve usare una FSM?

**Integrazione:** Per la playlist `.m3u8` la Macchina a Stati Finiti è un *overkill* (un'esagerazione).
A differenza dell'header HTTP (che ha una struttura dinamica chiave-valore sulla stessa riga), il file `.m3u8` è strettamente **line-based** (basato su righe intere).

La strategia migliore qui è un semplice parser a righe:

1. Scorri il buffer cercando il carattere `\n`.
2. Isoli la riga.
3. Usi una banale `strncmp(riga, "#EXTINF:", 8)` o `strncmp(riga, "#EXT-X-MEDIA-SEQUENCE:", 22)`.
4. Se la riga non inizia con `#`, la tratti direttamente come l'URL del chunk.
È molto più veloce da scrivere e computazionalmente efficientissimo.

### Il dilemma della Pausa: Cosa succede dopo 1 minuto o 12 minuti?

Questo è un punto critico. Quando l'utente clicca **Pausa**, l'Audio Thread si ferma, ma il Network Thread cosa fa?

* **Scenario A (Pausa di 1 minuto):** Il tuo Ring Buffer da 11 minuti ha spazio. Il Network Thread continua a scaricare chunk in background fino a riempire tutti i 675 secondi di memoria, poi si mette in stand-by (usando le condition variables). Quando l'utente preme **Play**, l'audio riparte **all'istante**, zero latenza, e l'utente riprende esattamente da dove aveva interrotto (Time-shifting).
* **Scenario B (Pausa di 12 minuti):** Qui superiamo la capienza del Ring Buffer (11 minuti) e, soprattutto, la "pazienza" del server di Radio 24. Dopo 12 minuti, i chunk che avevi nel buffer o quelli che dovevi scaricare subito dopo **sono stati cancellati da Akamai**.
* *Cosa succede al Play?* Se provi a suonare i vecchi chunk nel buffer, potrai farlo per 11 minuti, ma quando il Network Thread proverà a scaricare il chunk successivo, riceverà un errore `404 Not Found` dal server.
* *La Soluzione:* Devi implementare una **Pause Policy**. Se il tempo di pausa supera una certa soglia (es. 5 minuti), al click del tasto Play il software deve fare un **Flush** (svuotamento) del Ring Buffer, riscaricare il Master Playlist e riagganciarsi al "Live Edge" attuale della radio, ricominciando da zero.



### Serve un secondo buffer per i dati decodificati (PCM)?

**Sì, è tassativo.** Ne avrai assolutamente bisogno. Il motivo è il **mismatch (disallineamento) di dimensioni** tra come sputa i dati FFmpeg e come li richiede SDL3.

* **FFmpeg** decodifica l'audio a "pacchetti" chiamati `AVFrame`. Un frame HE-AAC contiene un blocco fisso di campioni (solitamente 1024 o 2048 campioni).
* **SDL3** (tramite la sua funzione di callback o l'Audio Stream) bussa alla porta del tuo programma e dice: *"Ehi, la scheda audio ha bisogno di esattamente 512 byte in questo millisecondo, dammeli"*.

Non puoi costringere FFmpeg a decodificare esattamente i byte chiesti da SDL3. Di conseguenza, l'architettura corretta prevede:

1. L'Audio Thread prende un chunk dal Ring Buffer primario.
2. FFmpeg lo decodifica e produce un `AVFrame` (PCM grezzo).
3. Spingi questo PCM grezzo dentro un **secondo Ring Buffer (Buffer PCM)** molto più piccolo (basta qualche Kilobyte).
4. SDL3 attinge direttamente e fluidamente da questo secondo buffer PCM. FFmpeg interverrà solo quando questo buffer si sta svuotando.

---

## 2. Integrazioni fondamentali da aggiungere al documento

Per rendere l'architettura a prova di bomba, devi aggiungere una sezione dedicata alla **Sincronizzazione dei Thread**:

### Mutex e Condition Variables (I semafori dei Thread)

Il Network Thread scrive nel Ring Buffer e l'Audio Thread legge dallo stesso Ring Buffer. Se non gestisci l'accesso, i due thread proveranno a toccare la stessa memoria nello stesso millisecondo, causando un crash inevitabile (`Race Condition`).

Nel tuo codice C dovrai implementare i thread POSIX (`pthread`) usando:

* **`pthread_mutex_t`**: Un lucchetto. Quando il Network Thread scrive un chunk, chiude il lucchetto. L'Audio Thread deve aspettare che il lucchetto si riapra per poter leggere.
* **`pthread_cond_t` (Condition Variables)**:
* Se il Ring Buffer è pieno (11 minuti), il Network Thread deve andare a dormire, svegliato dall'Audio Thread non appena quest'ultimo consuma un chunk.
* Se il Ring Buffer è vuoto (es. perdita di segnale), l'Audio Thread deve andare a dormire, svegliato dal Network Thread non appena un nuovo chunk viene scaricato.



---

## Tabella di Riepilogo dell'Architettura Aggiornata

| Componente | Input | Output | Struttura Dati |
| --- | --- | --- | --- |
| **Network Thread** | Rete HTTPS (Akamai) | Ring Buffer Primario | Line-based Parser + FSM HTTP |
| **Ring Buffer (5.6MB)** | Byte grezzi dei file `.ts` | Coda FIFO gestita da Mutex | Array circolare di strutture/puntatori |
| **Audio Thread** | Ring Buffer Primario | Buffer PCM Secondario | FFmpeg (`libavcodec`) |
| **Buffer PCM (Libreria)** | Campioni PCM decodificati | Canale Audio SDL3 | `AVAudioFifo` (incluso in FFmpeg) |
