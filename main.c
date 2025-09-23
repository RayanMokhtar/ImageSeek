#include "image.h"
#include <dirent.h>
#include <string.h>  

int main() {
    ImageFeatures feat_ref;  // Caractéristiques de l'image de référence
    char* filename_ref = "./archivePPMPGM/archive10pgm/arbre1.pgm";
    char* chemin_fichier_csv = "sortiefichier.csv";
    
    // Extraire les caractéristiques de l'image de référence
    if (extraire_features_from_file(filename_ref, &feat_ref, 0, 0.25, IMAGE_TYPE_PGM) != 0) {
        printf("Erreur extraction référence: %s\n", filename_ref);
        return 1;
    }
    printf("Référence extraite: %s (Largeur: %ld)\n", filename_ref, feat_ref.width);
    
    // Liste des répertoires à scanner
    const char* directories[] = {
        // "./archivePPMPGM/archive10ppm",
        "./archivePPMPGM/archive10pgm"
    };
    int num_dirs = sizeof(directories) / sizeof(directories[0]);
    
    // Poids pour evaluate_score (ajustez selon les tests)
    double weight_hist = 0.5;
    double weight_r = 0.05 , weight_g = 0.05, weight_b = 0.05;
    double weight_norm = 0.1, weight_contour = 0.2, weight_color = 0.1;
    DistanceFunc dist_func = distance_bhattacharyya ;  
    
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
            
            // Extraire les caractéristiques de l'image courante
            ImageFeatures feat_curr;
            if (extraire_features_from_file(full_path, &feat_curr, 0, 0.25, image_type) != 0) {
                printf("Erreur extraction: %s\n", full_path);
                continue;
            }
            
            // Calculer le score de similarité
            double score = evaluate_score(&feat_ref, &feat_curr, dist_func,
                                          weight_hist, weight_r, weight_g, weight_b,
                                          weight_norm, weight_contour, weight_color);
            
            // Afficher le résultat
            printf("Image: %s, Score: %.4f\n", full_path, score);
        }
        
        closedir(dir);
    }
    
    // // Sauvegarde en CSV (optionnel, pour l'image de référence)
    // FILE *f = fopen(chemin_fichier_csv, "a");
    // if (f) {
    //     // ecrire_csv_header(f); // Si besoin d'écrire l'en-tête une fois
    //     ecrire_csv_ligne(f, filename_ref, &feat_ref);
    //     fclose(f);
    //     printf("CSV écrit dans %s\n", chemin_fichier_csv);
    // } else {
    //     printf("Erreur ouverture CSV\n");
    // }
    
    return 0;
}