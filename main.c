#include "image.h"
#include <dirent.h>
#include <string.h>
#include <stdlib.h> 

//score + image
typedef struct {
    char filename[512];
    double score;
} ImageData;


//à déplacer dans utils 
int compare_scores(const void *a, const void *b) {
    double score_a = ((ImageData *)a)->score;
    double score_b = ((ImageData *)b)->score;
    if (score_a < score_b) return -1;
    if (score_a > score_b) return 1;
    return 0;
}
//servira pour ranking 
void sort_ranking(ImageData *data, int num_images) {
    qsort(data, num_images, sizeof(ImageData), compare_scores);
}

int main() {
    ImageFeatures feat_ref;  //image ref descripteurs
    char* filename_ref = "./archivePPMPGM/archive500ppm/2.ppm"; // chemin image de base
    
    //caractéristiques images base extraite
    if (extraire_features_from_file(filename_ref, &feat_ref, 0, 0.25, IMAGE_TYPE_PPM) != 0) {
        printf("Erreur extraction référence: %s\n", filename_ref);
        return 1;
    }
    printf("Référence extraite: %s (Largeur: %ld)\n", filename_ref, feat_ref.width);
    
    //répertoire à scanner 
    const char* directories[] = {
        "./archivePPMPGM/archive500ppm"
        // "./archivePPMPGM/archive500pgm" // à voir si on concaténe les deux ? 
    };
    int num_dirs = sizeof(directories) / sizeof(directories[0]);
    
    //TODO , ajuster les poids



    double weight_hist = 0.5;
    double weight_r =0.200, weight_g = 0.2 , weight_b = 0.3;
    double weight_norm = 0.1, weight_contour = 0.1, weight_color = 0.05;
    DistanceFunc dist_func = distance_bhattacharyya;
    




    //allocation mémoire pas très optimate mais idée score ces images 
    int max_images = 500;
    ImageData *score = malloc(max_images * sizeof(ImageData));
    if (score == NULL) {
        printf("Erreur allocation mémoire\n");
        return 1;
    }
    
    int num_images = 0;
    
    //parcours répertoires=> pour instant un seul donc pas utile
    for (int d = 0; d < num_dirs; d++) {
        DIR *dir = opendir(directories[d]);
        if (!dir) {
            printf("erreur => : %s\n", directories[d]);
            free(score);
            return 1;
        }
        
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            //traitement ici pour le split et récupérer nom fichier
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
            
            //chemin
            char full_path[512];
            snprintf(full_path, sizeof(full_path), "%s/%s", directories[d], entry->d_name);
            
            //récupérer chaque image => en général mm format que celui d'avant mais on garde flexibiltié
            int image_type = -1;
            if (strstr(entry->d_name, ".pgm")) image_type = IMAGE_TYPE_PGM;
            else if (strstr(entry->d_name, ".ppm")) image_type = IMAGE_TYPE_PPM;
            else continue;  // Ignorer les fichiers non image
            
            //curr c'est actuelle , à comparer avec ref plutot
            ImageFeatures feat_curr;
            if (extraire_features_from_file(full_path, &feat_curr, 0, 0.25, image_type) != 0) {
                printf("Erreur extraction: %s\n", full_path);
                continue;
            }
            
            //fct eval
            double current_score = evaluate_score(&feat_ref, &feat_curr, dist_func,
                                                 weight_hist, weight_r, weight_g, weight_b,
                                                 weight_norm, weight_contour, weight_color);
            
            //stockage avec strcpy
            strcpy(score[num_images].filename, full_path);
            score[num_images].score = current_score;
            num_images++;
        }
        
        closedir(dir);
    }
    
    //ranking
    sort_ranking(score, num_images);
    
    //affichage 
    printf("\n--- RANKING DES IMAGES (par score ascendant, plus petit = plus similaire) ---\n");
    printf("\n--- RANKING AVEC SCORES DÉTAILLÉS ---\n");
    for (int i = 0; i < num_images; i++) {
        printf("%d. %s: %.10f\n", i+1, score[i].filename, score[i].score);
    }
    
    //tableau libéré
    free(score);
    
    return 0;
}