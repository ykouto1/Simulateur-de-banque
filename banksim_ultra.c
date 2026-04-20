#include <stdio.h>
#include <stdlib.h>

#define MAX_COMPTES 100

typedef struct {
    int numero;
    float solde;
} Compte;

Compte comptes[MAX_COMPTES];
int totalComptes = 0;

/* Prototypes */
void afficherMenu();
void creerCompte();
void afficherComptes();
void supprimerCompte(int numero);
void consulterSolde(int numero);
void depot(int numero, float montant);
void retrait(int numero, float montant);

void afficherMenu() {
    printf("\n===== MENU BANCAIRE =====\n");
    printf("1. Creer un compte\n");
    printf("2. Afficher les comptes\n");
    printf("3. Depot\n");
    printf("4. Retrait\n");
    printf("5. Consulter solde\n");
    printf("6. Supprimer compte\n");
    printf("7. Quitter\n");
    printf("Choix : ");
}

void creerCompte() {

    if (totalComptes >= MAX_COMPTES) {
        printf("Limite de comptes atteinte.\n");
        return;
    }

    comptes[totalComptes].numero = totalComptes + 1;
    comptes[totalComptes].solde = 0;

    printf("Compte cree avec numero : %d\n", comptes[totalComptes].numero);

    totalComptes++;
}

void afficherComptes() {

    if (totalComptes == 0) {
        printf("Aucun compte.\n");
        return;
    }

    printf("\nListe des comptes :\n");

    for (int i = 0; i < totalComptes; i++) {
        printf("Compte %d : Solde = %.2f\n",
               comptes[i].numero,
               comptes[i].solde);
    }
}

void consulterSolde(int numero) {

    for (int i = 0; i < totalComptes; i++) {

        if (comptes[i].numero == numero) {
            printf("Solde : %.2f\n", comptes[i].solde);
            return;
        }
    }

    printf("Compte introuvable.\n");
}

void supprimerCompte(int numero) {

    for (int i = 0; i < totalComptes; i++) {

        if (comptes[i].numero == numero) {

            for (int j = i; j < totalComptes - 1; j++) {
                comptes[j] = comptes[j + 1];
            }

            totalComptes--;

            printf("Compte supprime.\n");
            return;
        }
    }

    printf("Compte introuvable.\n");
}

void depot(int numero, float montant) {

    for (int i = 0; i < totalComptes; i++) {

        if (comptes[i].numero == numero) {
            comptes[i].solde += montant;
            printf("Depot effectue.\n");
            return;
        }
    }

    printf("Compte introuvable.\n");
}

void retrait(int numero, float montant) {

    for (int i = 0; i < totalComptes; i++) {

        if (comptes[i].numero == numero) {

            if (comptes[i].solde >= montant) {
                comptes[i].solde -= montant;
                printf("Retrait effectue.\n");
            } else {
                printf("Solde insuffisant.\n");
            }

            return;
        }
    }

    printf("Compte introuvable.\n");
}

int main() {

    int choix;
    int numero;
    float montant;

    while (1) {

        afficherMenu();
        scanf("%d", &choix);

        switch (choix) {

            case 1:
                creerCompte();
                break;

            case 2:
                afficherComptes();
                break;

            case 3:
                printf("Numero du compte : ");
                scanf("%d", &numero);

                printf("Montant : ");
                scanf("%f", &montant);

                depot(numero, montant);
                break;

            case 4:
                printf("Numero du compte : ");
                scanf("%d", &numero);

                printf("Montant : ");
                scanf("%f", &montant);

                retrait(numero, montant);
                break;

            case 5:
                printf("Numero du compte : ");
                scanf("%d", &numero);

                consulterSolde(numero);
                break;

            case 6:
                printf("Numero du compte : ");
                scanf("%d", &numero);

                supprimerCompte(numero);
                break;

            case 7:
                printf("Au revoir !\n");
                return EXIT_SUCCESS;

            default:
                printf("Choix invalide\n");
        }
    }
}
