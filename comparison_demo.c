#include "image.h"
#include <dirent.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
    char filename[256];
    double score_old;
    double score_new;
} ComparisonResult;

int compare_results(const void *a, const void *b) {
    ComparisonResult *ra = (ComparisonResult *)a;
    ComparisonResult *rb = (ComparisonResult *)b;
    if (ra->score_new < rb->score_new) return -1;
    if (ra->score_new > rb->score_new) return 1;
    return 0;
}

int main() {
    ImageFeatures feat_ref;
    char* filename_ref = "./archivePPMPGM/archive10pgm/vache1.pgm";
    
    // Extraire les caractéristiques de l'image de référence
    if (extraire_features_from_file(filename_ref, &feat_ref, 0, 0.25, IMAGE_TYPE_PGM) != 0) {
        printf("Erreur extraction référence: %s\n", filename_ref);
        return 1;
    }
    
    printf("=== COMPARAISON ANCIEN vs NOUVEAU SYSTÈME ===\n");
    printf("Image de référence: %s\n\n", filename_ref);
    
    // Configuration ancien système
    double weight_hist = 0.5;
    double weight_r = 0.05, weight_g = 0.05, weight_b = 0.05;
    double weight_norm = 0.1, weight_contour = 0.2, weight_color = 0.1;
    DistanceFunc dist_func_old = distance_bhattacharyya;
    
    // Configuration nouveau système
    AdaptiveWeights adaptive_weights = calculer_poids_adaptatifs(&feat_ref);
    DistanceFunc dist_func_new = distance_earth_mover;
    
    const char* directories[] = {"./archivePPMPGM/archive10pgm"};
    int num_dirs = 1;
    
    ComparisonResult results[20];
    int result_count = 0;
    
    // Parcourir les images
    for (int d = 0; d < num_dirs; d++) {
        DIR *dir = opendir(directories[d]);
        if (!dir) continue;
        
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL && result_count < 20) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
            if (!strstr(entry->d_name, ".pgm")) continue;
            
            char full_path[512];
            snprintf(full_path, sizeof(full_path), "%s/%s", directories[d], entry->d_name);
            
            ImageFeatures feat_curr;
            if (extraire_features_from_file(full_path, &feat_curr, 0, 0.25, IMAGE_TYPE_PGM) != 0) {
                continue;
            }
            
            // Score ancien système
            double score_old = evaluate_score(&feat_ref, &feat_curr, dist_func_old,
                                            weight_hist, weight_r, weight_g, weight_b,
                                            weight_norm, weight_contour, weight_color);
            
            // Score nouveau système
            double score_new = evaluate_score_enhanced(&feat_ref, &feat_curr, 
                                                     dist_func_new, &adaptive_weights);
            
            // Sauvegarder résultat
            strncpy(results[result_count].filename, entry->d_name, 255);
            results[result_count].score_old = score_old;
            results[result_count].score_new = score_new;
            result_count++;
        }
        closedir(dir);
    }
    
    // Trier par nouveau score
    qsort(results, result_count, sizeof(ComparisonResult), compare_results);
    
    printf("Rang\tImage\t\t\tScore Ancien\tScore Nouveau\tAmélioration\n");
    printf("----\t-----\t\t\t------------\t-------------\t------------\n");
    
    for (int i = 0; i < result_count; i++) {
        double improvement = ((results[i].score_old - results[i].score_new) / results[i].score_old) * 100.0;
        if (results[i].score_old == 0.0) improvement = 0.0; // Éviter division par zéro
        
        printf("%d\t%-20s\t%.4f\t\t%.4f\t\t%+.1f%%\n", 
               i+1, results[i].filename, results[i].score_old, 
               results[i].score_new, improvement);
    }
    
    printf("\n=== ANALYSE ===\n");
    printf("- Plus le score est bas, plus l'image est similaire à la référence\n");
    printf("- Le système amélioré utilise des poids adaptatifs et de nouvelles métriques\n");
    printf("- Distance: Ancien=%s, Nouveau=%s\n", "Bhattacharyya", "Earth Mover's");
    
    return 0;
}
