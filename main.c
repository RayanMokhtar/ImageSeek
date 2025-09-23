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
    printf("=== Image de Référence ===\n");
    printf("Fichier: %s\n", filename_ref);
    printf("Dimensions: %ld x %ld\n", feat_ref.width, feat_ref.height);
    printf("Est couleur: %s\n", feat_ref.est_couleur ? "Oui" : "Non");
    printf("Moyenne gradient: %.4f\n", feat_ref.moyenne_gradient_norme);
    printf("Densité contours: %.4f\n", feat_ref.densite_contours);
    printf("Variance histogramme: %.2f\n", feat_ref.hist_variance);
    printf("Asymétrie: %.4f\n", feat_ref.hist_skewness);
    printf("Aplatissement: %.4f\n", feat_ref.hist_kurtosis);
    printf("Entropie: %.4f\n", feat_ref.hist_entropy);
    printf("Variance gradients: %.2f\n", feat_ref.gradient_variance);
    printf("Cohérence contours: %.4f\n", feat_ref.contour_coherence);
    if (feat_ref.est_couleur) {
        printf("Ratios RGB: R=%.3f G=%.3f B=%.3f\n", 
               feat_ref.ratio_rouge, feat_ref.ratio_vert, feat_ref.ratio_bleu);
    }
    printf("=========================\n\n");
    
    // Liste des répertoires à scanner
    const char* directories[] = {
        // "./archivePPMPGM/archive10ppm",
        "./archivePPMPGM/archive10pgm"
    };
    int num_dirs = sizeof(directories) / sizeof(directories[0]);
    
    // Calcul des poids adaptatifs basés sur l'image de référence
    AdaptiveWeights adaptive_weights = calculer_poids_adaptatifs(&feat_ref);
    
    // Configuration des fonctions de distance (testez différentes fonctions)
    DistanceFunc dist_func = distance_earth_mover; // Nouvelle distance EMD
    
    printf("=== Configuration Adaptative ===\n");
    printf("Poids histogramme global: %.3f\n", adaptive_weights.weight_hist_global);
    printf("Poids histogramme local: %.3f\n", adaptive_weights.weight_hist_local);
    printf("Poids moments: %.3f\n", adaptive_weights.weight_moments);
    printf("Poids texture: %.3f\n", adaptive_weights.weight_texture);
    printf("Poids contours: %.3f\n", adaptive_weights.weight_contour);
    printf("================================\n\n");  
    
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
            
            // Calculer le score de similarité avec la nouvelle fonction améliorée
            double score = evaluate_score_enhanced(&feat_ref, &feat_curr, dist_func, &adaptive_weights);
            
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