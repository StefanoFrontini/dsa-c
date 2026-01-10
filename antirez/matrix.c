#include <stdio.h>
#include <string.h>

int main(void)
{
    int is_admin = 0; // Variabile intera (4 byte)
    char buffer[10];  // Array di caratteri (10 byte)

    printf("Indirizzo di is_admin: %p\n", &is_admin);
    printf("Indirizzo di buffer:   %p\n", &buffer);

    printf("\nInserisci la password: ");
    // LEGGIAMO L'INPUT IN MODO NON SICURO
    // scanf("%s") non controlla se scrivi più di 10 caratteri!
    scanf("%s", buffer);

    if (is_admin != 0)
    {
        printf("\n[SUCCESS] Accesso Admin garantito! (is_admin = 0x%x)\n", is_admin);
    }
    else
    {
        printf("\n[FAIL] Accesso negato. Sei un utente normale.\n");
    }

    return 0;
}