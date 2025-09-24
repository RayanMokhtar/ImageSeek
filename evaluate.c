#include "image.h"
#include <dirent.h>
#include <string.h>

// Structure pour stocker nom et score d'une image
typedef struct {
    char filename[512];
    double score;
} ImageData;


int compare_scores(const void *a, const void *b) {
    double score_a = ((ImageData *)a)->score;
    double score_b = ((ImageData *)b)->score;
    if (score_a < score_b) return -1;
    if (score_a > score_b) return 1;
    return 0;
}


void sort_ranking(ImageData *data, int num_images) {
    qsort(data, num_images, sizeof(ImageData), compare_scores);
}

int main() {
    ImageFeatures feat_ref;  // Caractéristiques de l'image de référence
    char* filename_ref = "./archivePPMPGM/archive10pgm/arbre2.pgm";
    char* chemin_fichier_csv = "sortiefichier.csv";
    
    // Extraire les caractéristiques de l'image de référence
    if (extraire_features_from_file(filename_ref, &feat_ref, 0, 0.25, IMAGE_TYPE_PGM) != 0) {
        printf("Erreur extraction référence: %s\n", filename_ref);
        return 1;
    }
    printf("Référence extraite: %s (Largeur: %ld)\n", filename_ref, feat_ref.width);
    
    // Liste des répertoires à scanner
    const char* directories[] = {
        "./archivePPMPGM/archive10pgm"
    };
    int num_dirs = sizeof(directories) / sizeof(directories[0]);
    
    //TODO , ajuster les poids
    double weight_hist = 0.5;
    double weight_r =0.200, weight_g = 0.2 , weight_b = 0.3;
    double weight_norm = 0.1, weight_contour = 0.1, weight_color = 0.05;
    DistanceFunc dist_func = distance_bhattacharyya;
    ImageData score[100];  //tableau de scoires
    int num_images = 0;
    
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
            double current_score = evaluate_score(&feat_ref, &feat_curr, dist_func,
                                                 weight_hist, weight_r, weight_g, weight_b,
                                                 weight_norm, weight_contour, weight_color);
            
            // Stocker dans le tableau score
            strcpy(score[num_images].filename, full_path);
            score[num_images].score = current_score;
            num_images++;
            
            // Vérifier la taille max
            if (num_images >= 100) {
                printf("Trop d'images, arrêt à 100\n");
                break;
            }
        }
        
        closedir(dir);
        if (num_images >= 100) break;
    }
    
    //ranking
    sort_ranking(score, num_images);
    
    //affihage 
    printf("\n--- RANKING DES IMAGES (par score ascendant, plus petit = plus similaire) ---\n");
    for (int i = 0; i < num_images; i++) {
        printf("%d. Image: %s, Score: %.4f\n", i + 1, score[i].filename, score[i].score);
    }
    
    
    return 0;
}