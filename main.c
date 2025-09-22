// filepath: c:\Users\mokht\Desktop\indexation_image\ImageSeek\main.c
#include "image.h"

int main() {
    ImageFeatures feat;
    char* filename = "./archivePPMPGM/archive10pgm/arbre1.pgm";
    char* chemin_fichier_csv = "sortiefichier.csv";
    if (extraire_features_from_file(filename, &feat, 0, 0.25, IMAGE_TYPE_PGM) == 0) {
        printf("Largeur: %ld\n", feat.width);
    }
    //sauvegarde dans un fichier
    FILE *f = fopen(chemin_fichier_csv,"a");
    if (f) {
            // ecrire_csv_header(f); // pas besoin de réécrire chef 
            ecrire_csv_ligne(f, filename, &feat);
            fclose(f);
            printf("CSV écrit dans %s\n", chemin_fichier_csv);
        } else {
            printf("Erreur ouverture CSV\n");
        }

    return 0;
}