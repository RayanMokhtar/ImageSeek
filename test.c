#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <math.h>
#include <ctype.h>  // Pour isdigit dans extract_category
#include "image.h"

// Structure pour une image dans le dataset
typedef struct {
    char filename[256];     // Nom du fichier (ex. "arbre1.pgm")
    char category[256];     // Catégorie extraite (ex. "arbre")
    ImageFeatures feat;     // Caractéristiques extraites (histogrammes, contours, etc.)
} DatasetImage;

/*
 * Extrait la catégorie d'un nom de fichier.
 * Exemple : "arbre1.pgm" -> "arbre"
 * Logique : Enlève l'extension, puis enlève les chiffres à la fin.
 */
void extract_category(const char *filename, char *category) {
    strcpy(category, filename);
    char *dot = strrchr(category, '.');  // Trouve le dernier '.'
    if (dot) *dot = '\0';  // Enlève l'extension (.pgm)
    char *num = category;
    while (*num && !isdigit(*num)) num++;  // Avance jusqu'aux chiffres
    if (num > category) *(num - 1) = '\0';  // Enlève les chiffres et le caractère précédent
}

/*
 * Charge le dataset depuis un dossier.
 * - Compte les fichiers .pgm.
 * - Alloue un tableau de DatasetImage.
 * - Pour chaque fichier, extrait la catégorie et les features.
 * Retour : 0 si succès, -1 si erreur (dossier introuvable).
 */
int load_dataset(const char *dir_path, DatasetImage **dataset, int *num_images) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        printf("Erreur : Impossible d'ouvrir le dossier %s\n", dir_path);
        return -1;
    }

    struct dirent *entry;
    *num_images = 0;
    // Premier passage : compte les fichiers .pgm
    while ((entry = readdir(dir))) {
        if (strstr(entry->d_name, ".ppm")) (*num_images)++;
    }
    rewinddir(dir);  // Remet au début

    // Allocation du tableau
    *dataset = malloc(*num_images * sizeof(DatasetImage));
    if (!*dataset) {
        printf("Erreur : Allocation mémoire échouée\n");
        closedir(dir);
        return -1;
    }

    int idx = 0;
    // Deuxième passage : charge les données
    while ((entry = readdir(dir)) && idx < *num_images) {
        if (strstr(entry->d_name, ".ppm")) {
            strcpy((*dataset)[idx].filename, entry->d_name);
            extract_category(entry->d_name, (*dataset)[idx].category);
            char full_path[512];
            sprintf(full_path, "%s/%s", dir_path, entry->d_name);
            // Charge les features ; si échec, ignore l'image
            if (extraire_features_from_file(full_path, &(*dataset)[idx].feat, 0, 0.25, IMAGE_TYPE_PPM) != 0) {
                printf("Erreur extraction features pour %s\n", full_path);
                idx--;  // Ne compte pas cette image
            }
            idx++;
        }
    }
    closedir(dir);
    *num_images = idx;  // Ajuste si des images ont été ignorées
    printf("Dataset chargé : %d images\n", *num_images);
    return 0;
}

/*
 * Trouve les indices des images similaires (même catégorie, différent de la requête).
 * Exemple : Pour "arbre1", retourne indices de "arbre2", "arbre3".
 */
void get_similar_indices(const DatasetImage *dataset, int num_images, int query_idx, int *similar_indices, int *num_similar) {
    *num_similar = 0;
    for (int i = 0; i < num_images; i++) {
        if (i != query_idx && strcmp(dataset[i].category, dataset[query_idx].category) == 0) {
            similar_indices[*num_similar] = i;
            (*num_similar)++;
        }
    }
}

/*
 * Calcule le top-k images les plus similaires (plus petits scores) pour une requête.
 * - Exclut soi-même.
 * - Utilise evaluate_score avec les poids donnés.
 * - Tri simple : trouve le min k fois.
*/
void get_top_k(const DatasetImage *dataset, int num_images, int query_idx, DistanceFunc dist_func,
               double w_hist, double w_r, double w_g, double w_b, double w_norm, double w_contour, double w_color,
               int k, int *top_k_indices) {
    double *scores = malloc(num_images * sizeof(double));
    if (!scores) {
        printf("Erreur : Allocation scores échouée\n");
        return;
    }
    // Calcule les scores pour toutes les images
    for (int i = 0; i < num_images; i++) {
        if (i == query_idx) {
            scores[i] = INFINITY;  // Exclut soi-même
        } else {
            scores[i] = evaluate_score(&dataset[query_idx].feat, &dataset[i].feat, dist_func,
                                       w_hist, w_r, w_g, w_b, w_norm, w_contour, w_color);
        }
    }

    // Tri pour top-k : trouve les k plus petits scores
    for (int i = 0; i < k; i++) {
        double min_score = INFINITY;
        int min_idx = -1;
        for (int j = 0; j < num_images; j++) {
            if (scores[j] < min_score) {
                min_score = scores[j];
                min_idx = j;
            }
        }
        if (min_idx != -1) {
            top_k_indices[i] = min_idx;
            scores[min_idx] = INFINITY;  // Marque comme traité
        }
    }
    free(scores);
}

/*
 * Évalue l'accuracy pour une requête : 1.0 si au moins un similaire est dans top-k, 0.0 sinon.
 */
double evaluate_query(const DatasetImage *dataset, int num_images, int query_idx, DistanceFunc dist_func,
                      double w_hist, double w_r, double w_g, double w_b, double w_norm, double w_contour, double w_color, int k) {
    int similar_indices[10];  // Max 10 similaires (suffisant pour 10 images)
    int num_similar;
    get_similar_indices(dataset, num_images, query_idx, similar_indices, &num_similar);

    int top_k_indices[10];  // k <= 10
    get_top_k(dataset, num_images, query_idx, dist_func, w_hist, w_r, w_g, w_b, w_norm, w_contour, w_color, k, top_k_indices);

    // Vérifie si un similaire est dans top-k
    for (int i = 0; i < k; i++) {
        for (int j = 0; j < num_similar; j++) {
            if (top_k_indices[i] == similar_indices[j]) return 1.0;
        }
    }
    return 0.0;
}

/*
 * Grid search pour optimiser les poids.
 * - Explore toutes les combinaisons de poids dans les grilles.
 * - Calcule l'accuracy moyenne sur toutes les requêtes pour chaque combinaison.
 * - Garde la meilleure (accuracy max, puis score_total min si égalité).
 * - Affiche les poids optimaux, accuracy, et analyse des échecs.
 */
void grid_search_cv(const char *dir_path, DistanceFunc dist_func, int k) {
    DatasetImage *dataset;
    int num_images;
    if (load_dataset(dir_path, &dataset, &num_images) != 0) {
        return;  // Erreur déjà affichée
    }

    
    double hist_vals[] = {0.5, 1.0, 1.5, 2.0};     // 5 valeurs (suppression de 2.5)
    double rgb_vals[] = {0.05, 0.1, 0.2, 0.3};     // 5 valeurs (suppression de 0.5)
    double norm_vals[] = {0.1, 0.2, 0.3, 0.4};     // 5 valeurs (suppression de 0.5)
    double contour_vals[] = {0.1, 0.2, 0.3};        // 4 valeurs (suppression de 0.5)
    double color_vals[] = {0.05, 0.1, 0.2, 0.3}; 
    
    int nh = sizeof(hist_vals) / sizeof(double);
    int nr = sizeof(rgb_vals) / sizeof(double);
    int nn = sizeof(norm_vals) / sizeof(double);
    int nc = sizeof(contour_vals) / sizeof(double);
    int nco = sizeof(color_vals) / sizeof(double);

    double best_accuracy = 0.0;
    double best_w_hist = 0.0, best_w_r = 0.0, best_w_g = 0.0, best_w_b = 0.0;
    double best_w_norm = 0.0, best_w_contour = 0.0, best_w_color = 0.0;
    double best_score_total = INFINITY;

    int total_combinations = nh * nr * nr * nr * nn * nc * nco;
    int current_combination = 0;

    // Boucles imbriquées pour explorer toutes les combinaisons
    for (int ih = 0; ih < nh; ih++) {
        for (int ir = 0; ir < nr; ir++) {
            for (int ig = 0; ig < nr; ig++) {  // G et B utilisent la même grille
                for (int ib = 0; ib < nr; ib++) {
                    for (int inn = 0; inn < nn; inn++) {
                        for (int ico = 0; ico < nc; ico++) {
                            for (int icol = 0; icol < nco; icol++) {
                                current_combination++;
                                if (current_combination % 5000 == 0) {  // Affichage moins fréquent pour moins de spam
                                    printf("Progression : %d/%d combinaisons traitées (%.1f%%)\n", current_combination, total_combinations, (double)current_combination / total_combinations * 100);
                                }

                                double accuracy = 0.0;
                                double score_total = 0.0;
                                // Évalue sur toutes les requêtes
                                for (int q = 0; q < num_images; q++) {
                                    double acc = evaluate_query(dataset, num_images, q, dist_func,
                                                               hist_vals[ih], rgb_vals[ir], rgb_vals[ig], rgb_vals[ib],
                                                               norm_vals[inn], contour_vals[ico], color_vals[icol], k);
                                    accuracy += acc;
                                    // Score total amélioré : moyenne des scores entre la requête et toutes les autres (plus représentatif)
                                    double local_score = 0.0;
                                    int count = 0;
                                    for (int other = 0; other < num_images; other++) {
                                        if (other != q) {
                                            local_score += evaluate_score(&dataset[q].feat, &dataset[other].feat, dist_func,
                                                                          hist_vals[ih], rgb_vals[ir], rgb_vals[ig], rgb_vals[ib],
                                                                          norm_vals[inn], contour_vals[ico], color_vals[icol]);
                                            count++;
                                        }
                                    }
                                    score_total += local_score / count;
                                }
                                accuracy /= num_images;
                                score_total /= num_images;

                                // Met à jour le meilleur si mieux
                                if (accuracy > best_accuracy || (accuracy == best_accuracy && score_total < best_score_total)) {
                                    best_accuracy = accuracy;
                                    best_score_total = score_total;
                                    best_w_hist = hist_vals[ih];
                                    best_w_r = rgb_vals[ir];
                                    best_w_g = rgb_vals[ig];
                                    best_w_b = rgb_vals[ib];
                                    best_w_norm = norm_vals[inn];
                                    best_w_contour = contour_vals[ico];
                                    best_w_color = color_vals[icol];
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Affichage des résultats optimaux
    printf("\n--- RÉSULTATS OPTIMAUX ---\n");
    printf("Score total moyen : %.4f\n", best_score_total);
    printf("Meilleurs poids :\n");
    printf("  Hist: %.3f (histogramme)\n", best_w_hist);
    printf("  R: %.3f, G: %.3f, B: %.3f (canaux RGB)\n", best_w_r, best_w_g, best_w_b);
    printf("  Norm: %.3f (norme du gradient)\n", best_w_norm);
    printf("  Contour: %.3f (densité des contours)\n", best_w_contour);
    printf("  Color: %.3f (features couleur globale)\n", best_w_color);
    printf("Accuracy : %.3f (%d/%d requêtes réussies)\n", best_accuracy, (int)(best_accuracy * num_images), num_images);

    // Sauvegarde des meilleurs poids dans un fichier
    FILE *file = fopen("best_weights.txt", "w");
    if (file) {
        fprintf(file, "Meilleurs poids pour evaluate_score :\n");
        fprintf(file, "Hist: %.3f\n", best_w_hist);
        fprintf(file, "R: %.3f\n", best_w_r);
        fprintf(file, "G: %.3f\n", best_w_g);
        fprintf(file, "B: %.3f\n", best_w_b);
        fprintf(file, "Norm: %.3f\n", best_w_norm);
        fprintf(file, "Contour: %.3f\n", best_w_contour);
        fprintf(file, "Color: %.3f\n", best_w_color);
        fprintf(file, "Accuracy: %.3f\n", best_accuracy);
        fclose(file);
        printf("Poids sauvegardés dans 'best_weights.txt'\n");
    } else {
        printf("Erreur : Impossible de sauvegarder les poids\n");
    }

    // Analyse détaillée des échecs
    printf("\n--- ANALYSE DES ÉCHECS ---\n");
    printf("Requêtes où aucun similaire n'est dans top-%d :\n", k);
    int failed_count = 0;
    for (int q = 0; q < num_images; q++) {
        double acc = evaluate_query(dataset, num_images, q, dist_func,
                                    best_w_hist, best_w_r, best_w_g, best_w_b, best_w_norm, best_w_contour, best_w_color, k);
        if (acc == 0.0) {
            printf("  Échec pour %s (catégorie: %s)\n", dataset[q].filename, dataset[q].category);
            printf("    Attendu (similaires) : ");
            int similar_indices[10];
            int num_similar;
            get_similar_indices(dataset, num_images, q, similar_indices, &num_similar);
            for (int j = 0; j < num_similar; j++) {
                printf("%s ", dataset[similar_indices[j]].filename);
            }
            printf("\n    Retourné (top-%d) : ", k);
            int top_k_indices[10];
            get_top_k(dataset, num_images, q, dist_func, best_w_hist, best_w_r, best_w_g, best_w_b, best_w_norm, best_w_contour, best_w_color, k, top_k_indices);
            for (int i = 0; i < k; i++) {
                printf("%s ", dataset[top_k_indices[i]].filename);
            }
            printf("\n");
            failed_count++;
        }
    }
    printf("Total échecs : %d/%d\n", failed_count, num_images);
    if (failed_count > 0) {
        printf("Raison probable : Features insuffisantes pour distinguer ces catégories (ex. bus/mer confondus avec arbres).\n");
        printf("Suggestion : Augmenter k, ajuster seuil_contour, ou utiliser plus de features.\n");
    }

    free(dataset);
}

/*
 * Fonction principale : Lance le grid search.
 */
int main() {
    const char *dir_path = "./archivePPMPGM/archive10ppm";  // Dossier du dataset
    DistanceFunc dist_func = distance_bhattacharyya;       // Fonction de distance (peut être changée)
    int k = 2;                                             // Top-k pour l'évaluation
    grid_search_cv(dir_path, dist_func, k);
    return 0;
}