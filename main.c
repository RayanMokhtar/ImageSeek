#include "image.h"
#include <dirent.h>
#include <string.h>
#include <libgen.h>  // Pour basename (si disponible, sinon manuel)

// Structure pour stocker nom et features d'une image
typedef struct {
    char filename[256];
    ImageFeatures feat;
} ImageData;

void write_image_to_csv(FILE *f, const char *filename, ImageFeatures *feat) {
    // Construire la chaîne pour l'histogramme : "[val0,val1,...,val255]"
    char hist_str[4096];  // Buffer assez grand pour 256 valeurs (environ 6 chars chacune + virgules + crochets)
    sprintf(hist_str, "[");  // Début du tableau JSON
    for (int i = 0; i < 256; i++) {
        char val_str[32];
        sprintf(val_str, "%.6f", feat->hist[i]);
        strcat(hist_str, val_str);
        if (i < 255) strcat(hist_str, ",");  // Virgule entre éléments
    }
    strcat(hist_str, "]");  // Fin du tableau JSON
    
    fprintf(f, "\"%s\",%ld,%ld,%.6f,%.6f,%.6f,%.6f,%.6f,%d,\"%s\"\n",
            filename,
            feat->width,  // Largeur de l'image
            feat->height,  // Hauteur de l'image
            feat->moyenne_gradient_norme,  // Moyenne de la norme du gradient
            feat->densite_contours,  // Densité des contours
            feat->ratio_rouge,  // Ratio rouge
            feat->ratio_vert,  // Ratio vert
            feat->ratio_bleu,  // Ratio bleu
            feat->est_couleur,  // Indicateur couleur (0 ou 1)
            hist_str  // Histogramme comme tableau JSON
    );
}

int main() {
    char* chemin_fichier_csv = "features_all.csv";  // Changé pour refléter toutes les features
    
    // Liste des répertoires à scanner
    const char* directories[] = {
        "./archivePPMPGM/archive500ppm"
    };
    int num_dirs = sizeof(directories) / sizeof(directories[0]);
    
    // Ouvrir le fichier CSV pour écriture
    FILE *f = fopen(chemin_fichier_csv, "w");
    if (!f) {
        printf("Erreur ouverture CSV: %s\n", chemin_fichier_csv);
        return 1;
    }
    
    // Écrire l'en-tête CSV : "filename,width,height,moyenne_gradient_norme,densite_contours,ratio_rouge,ratio_vert,ratio_bleu,is_color,histogram"
    fprintf(f, "filename,width,height,moyenne_gradient_norme,densite_contours,ratio_rouge,ratio_vert,ratio_bleu,est_couleur,histogram\n");
    
    // Parcourir chaque répertoire
    for (int d = 0; d < num_dirs; d++) {
        DIR *dir = opendir(directories[d]);
        if (!dir) {
            printf("Erreur ouverture répertoire: %s\n", directories[d]);
            continue;
        }
        
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            // Ignorer . et ..
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
            
            // Construire le chemin complet
            char full_path[512];
            snprintf(full_path, sizeof(full_path), "%s/%s", directories[d], entry->d_name);
            
            // Déterminer le type d'image
            int image_type = -1;
            if (strstr(entry->d_name, ".pgm")) image_type = IMAGE_TYPE_PGM;
            else if (strstr(entry->d_name, ".ppm")) image_type = IMAGE_TYPE_PPM;
            else continue;  // Ignorer les fichiers non image
            
            // Extraire les caractéristiques de l'image
            ImageFeatures feat;
            if (extraire_features_from_file(full_path, &feat, 0, 0.25, image_type) != 0) {
                printf("Erreur extraction: %s\n", full_path);
                continue;
            }
            
            // Extraire le nom du fichier sans chemin (utilise basename si disponible, sinon manuel)
            char filename_only[256];
            strcpy(filename_only, entry->d_name);  
            
            // Appeler la fonction pour écrire la ligne CSV
            write_image_to_csv(f, filename_only, &feat);
            
            printf("Features sauvegardées pour: %s\n", filename_only);
        }
        
        closedir(dir);
    }
    
    fclose(f);
    printf("Toutes les features sauvegardées dans %s\n", chemin_fichier_csv);
    
    return 0;
}