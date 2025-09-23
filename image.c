#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "image.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Fonction utilitaire pour clamper une valeur int entre 0 et 255
static inline byte clampi(int v) {
    if (v < 0) return 0;
    if (v > 255) return 255;
    return (byte)v;
}

// Charge une image PGM en niveaux de gris
byte **load_pgm_gray(const char *filename, long *nrl, long *nrh, long *ncl, long *nch) {
    return LoadPGM_bmatrix((char*)filename, nrl, nrh, ncl, nch);
}

// Charge une image PPM couleur, convertit en gris, calcule ratios RGB et détecte couleur
rgb8 **load_ppm_rgb_and_to_gray(const char *filename,
                                long *nrl, long *nrh, long *ncl, long *nch,
                                byte ***pgray,
                                double *r_ratio, double *g_ratio, double *b_ratio,
                                int *is_color) {
    // Charge l'image PPM
    rgb8 **rgb = LoadPPM_rgb8matrix((char*)filename, nrl, nrh, ncl, nch);
    if (!rgb) return NULL;

    long i, j;
    //sortie matrice en niveau de gris 
    *pgray = bmatrix(*nrl, *nrh, *ncl, *nch);

    //initialisation
    uint64_t Rsum = 0, Gsum = 0, Bsum = 0;

    //parcours image
    for (i = *nrl; i <= *nrh; i++) {
        for (j = *ncl; j <= *nch; j++) {
            //
            Rsum += rgb[i][j].r;
            Gsum += rgb[i][j].g;
            Bsum += rgb[i][j].b;

            //conversion image en niveau de gris => et transformer image directement
            double y = 0.299 * rgb[i][j].r + 0.587 * rgb[i][j].g + 0.114 * rgb[i][j].b;
            (*pgray)[i][j] = clampi((int)(y + 0.5));  // Arrondi et clamp
        }
    }

    //calcule ratios normalisés
    double S = (double)Rsum + (double)Gsum + (double)Bsum;
    if (S <= 0.0) {
        *r_ratio = *g_ratio = *b_ratio = 1.0 / 3.0; 
    } else {
        *r_ratio = Rsum / S;
        *g_ratio = Gsum / S;
        *b_ratio = Bsum / S;
    }

    //détecte si image couleur est vraiment noir et blanc
    *is_color = verifier_image_couleur_est_nb(Rsum, Gsum, Bsum, *nrl, *nrh, *ncl, *nch);

    return rgb;
}

//sauvergarde image
void save_pgm_gray(const char *filename, byte **m, long nrl, long nrh, long ncl, long nch) {
    SavePGM_bmatrix(m, nrl, nrh, ncl, nch, (char*)filename);
}

//implémentation du filtre , choix notation c++ (in/out)
void filtre_moyenneur(byte **in, byte **out, long nrl, long nrh, long ncl, long nch) {
    long i, j;
    for (i = nrl + 1; i <= nrh - 1; i++) {
        for (j = ncl + 1; j <= nch - 1; j++) {
            //
            int s = 0;
            s += in[i-1][j-1] + in[i-1][j] + in[i-1][j+1];
            s += in[i  ][j-1] + in[i  ][j] + in[i  ][j+1];
            s += in[i+1][j-1] + in[i+1][j] + in[i+1][j+1];
            //moyenne de chaque puixel
            out[i][j] = (byte)(s / 9);
        }
    }
    //ici cas particulier comme la shape baisse, il faut remplacer par des 0 les bords
    for (i = nrl; i <= nrh; i++) {
        out[i][ncl] = in[i][ncl];
        out[i][nch] = in[i][nch];
    }
    for (j = ncl; j <= nch; j++) {
        out[nrl][j] = in[nrl][j];
        out[nrh][j] = in[nrh][j];
    }
}

// Calcule Ix et Iy avec Sobel
void sobel_ix_iy(byte **gray, int **ix, int **iy, long nrl, long nrh, long ncl, long nch) {
    //masque sobel 
    static const int SX[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};  // Horizontal
    static const int SY[3][3] = {{-1, -2, -1}, {0, 0, 0}, {1, 2, 1}};  // Vertical

    long i, j;
    //traite information
    for (i = nrl + 1; i <= nrh - 1; i++) {
        for (j = ncl + 1; j <= nch - 1; j++) {
            int gx = 0, gy = 0;

            //convoluer avec SX pour gx
            gx = SX[0][0] * gray[i-1][j-1] + SX[0][1] * gray[i-1][j] + SX[0][2] * gray[i-1][j+1] + 
            SX[1][0] * gray[i][j-1] + SX[1][1] * gray[i][j] + SX[1][2] * gray[i][j+1] +
            SX[2][0] * gray[i+1][j-1] + SX[2][1] * gray[i+1][j] + SX[2][2] * gray[i+1][j+1];

            //convoluer avec SY pour gy
            gy += SY[0][0] * gray[i-1][j-1] + SY[0][1] * gray[i-1][j] + SY[0][2] * gray[i-1][j+1]+ 
            SY[1][0] * gray[i][j-1] + SY[1][1] * gray[i][j] + SY[1][2] * gray[i][j+1]+
            SY[2][0] * gray[i+1][j-1] + SY[2][1] * gray[i+1][j] + SY[2][2] * gray[i+1][j+1];

            ix[i][j] = gx;
            iy[i][j] = gy;
        }
    }
    //bords à 0
    for (i = nrl; i <= nrh; i++) {
        ix[i][ncl] = ix[i][nch] = 0;
        iy[i][ncl] = iy[i][nch] = 0;
    }
    for (j = ncl; j <= nch; j++) {
        ix[nrl][j] = ix[nrh][j] = 0;
        iy[nrl][j] = iy[nrh][j] = 0;
    }
}

// Calcule la magnitude normalisée du gradient et retourne la moyenne
double gradient_magnitude_norm(byte **gray, double **mag_norm,
                               long nrl, long nrh, long ncl, long nch) {
    //Ix et Iy
    int **ix = imatrix(nrl, nrh, ncl, nch);
    int **iy = imatrix(nrl, nrh, ncl, nch);
    sobel_ix_iy(gray, ix, iy, nrl, nrh, ncl, nch);

    double sum = 0.0;
    long count = 0;
    // Calcule pour l'intérieur
    for (long i = nrl + 1; i <= nrh - 1; i++) {
        for (long j = ncl + 1; j <= nch - 1; j++) {
            double gx = (double)ix[i][j];
            double gy = (double)iy[i][j];
            double mag = sqrt(gx * gx + gy * gy);
            double mn = mag / VAL_SOBEL_MAX_THEORIQUE;  // Normalisation
            if (mn > 1.0) mn = 1.0;
            if (mn < 0.0) mn = 0.0;
            mag_norm[i][j] = mn;
            sum += mn;
            count++;
        }
    }
    // Bords à 0
    for (long i = nrl; i <= nrh; i++) {
        mag_norm[i][ncl] = mag_norm[i][nch] = 0.0;
    }
    for (long j = ncl; j <= nch; j++) {
        mag_norm[nrl][j] = mag_norm[nrh][j] = 0.0;
    }

    // Libère la mémoire
    free_imatrix(ix, nrl, nrh, ncl, nch);
    free_imatrix(iy, nrl, nrh, ncl, nch);

    // Retourne la moyenne
    return (count > 0) ? (sum / (double)count) : 0.0;
}

// Suppression de non-maxima pour affiner les contours
void suppression_non_maxima(double **mag_norm, int **ix, int **iy, 
                           double **mag_suppressed, long nrl, long nrh, long ncl, long nch) {
    for (long i = nrl + 1; i <= nrh - 1; i++) {
        for (long j = ncl + 1; j <= nch - 1; j++) {
            double mag = mag_norm[i][j];
            if (mag == 0.0) {
                mag_suppressed[i][j] = 0.0;
                continue;
            }
            
            // Calcul de l'angle du gradient
            double angle = atan2((double)iy[i][j], (double)ix[i][j]) * 180.0 / M_PI;
            if (angle < 0) angle += 180.0; // Normalisation 0-180°
            
            double q = 255.0, r = 255.0; // Valeurs des voisins à comparer
            
            // Quantification de la direction en 4 directions principales
            if ((0 <= angle && angle < 22.5) || (157.5 <= angle && angle <= 180)) {
                // Direction horizontale (0°)
                q = mag_norm[i][j+1];
                r = mag_norm[i][j-1];
            } else if (22.5 <= angle && angle < 67.5) {
                // Direction diagonale (45°)
                q = mag_norm[i+1][j-1];
                r = mag_norm[i-1][j+1];
            } else if (67.5 <= angle && angle < 112.5) {
                // Direction verticale (90°)
                q = mag_norm[i+1][j];
                r = mag_norm[i-1][j];
            } else if (112.5 <= angle && angle < 157.5) {
                // Direction diagonale (135°)
                q = mag_norm[i-1][j-1];
                r = mag_norm[i+1][j+1];
            }
            
            // Suppression si pas maximum local
            if (mag >= q && mag >= r) {
                mag_suppressed[i][j] = mag;
            } else {
                mag_suppressed[i][j] = 0.0;
            }
        }
    }
}

// Crée la carte des contours avec hystérésis améliorée et retourne la densité
double detection_contours_hysterisis(double **mag_norm, byte **edges,
                                     long nrl, long nrh, long ncl, long nch, double t_norm) {
    // Calcul des gradients pour la suppression de non-maxima
    int **ix = imatrix(nrl, nrh, ncl, nch);
    int **iy = imatrix(nrl, nrh, ncl, nch);
    
    // Note: On suppose que les gradients ont déjà été calculés dans mag_norm
    // Pour cette version, on utilise un calcul simple des gradients locaux
    for (long i = nrl + 1; i <= nrh - 1; i++) {
        for (long j = ncl + 1; j <= nch - 1; j++) {
            // Approximation des gradients à partir de mag_norm
            ix[i][j] = (int)((mag_norm[i][j+1] - mag_norm[i][j-1]) * 1000);
            iy[i][j] = (int)((mag_norm[i+1][j] - mag_norm[i-1][j]) * 1000);
        }
    }
    
    // Suppression de non-maxima
    double **mag_suppressed = dmatrix(nrl, nrh, ncl, nch);
    suppression_non_maxima(mag_norm, ix, iy, mag_suppressed, nrl, nrh, ncl, nch);
    
    // Hystérésis à double seuil
    double t_high = t_norm;
    double t_low = t_norm * 0.4; // Seuil bas = 40% du seuil haut
    
    byte **strong_edges = bmatrix(nrl, nrh, ncl, nch);
    byte **weak_edges = bmatrix(nrl, nrh, ncl, nch);
    
    // Classification des pixels en forts, faibles ou supprimés
    for (long i = nrl; i <= nrh; i++) {
        for (long j = ncl; j <= nch; j++) {
            if (mag_suppressed[i][j] >= t_high) {
                strong_edges[i][j] = 1;
                weak_edges[i][j] = 0;
            } else if (mag_suppressed[i][j] >= t_low) {
                strong_edges[i][j] = 0;
                weak_edges[i][j] = 1;
            } else {
                strong_edges[i][j] = 0;
                weak_edges[i][j] = 0;
            }
            edges[i][j] = strong_edges[i][j]; // Initialisation avec les contours forts
        }
    }
    
    // Propagation des contours faibles connectés aux contours forts
    int changed = 1;
    while (changed) {
        changed = 0;
        for (long i = nrl + 1; i <= nrh - 1; i++) {
            for (long j = ncl + 1; j <= nch - 1; j++) {
                if (weak_edges[i][j] == 1 && edges[i][j] == 0) {
                    // Vérifier s'il y a un contour fort dans le voisinage
                    for (int di = -1; di <= 1; di++) {
                        for (int dj = -1; dj <= 1; dj++) {
                            if (edges[i + di][j + dj] == 1) {
                                edges[i][j] = 1;
                                changed = 1;
                                break;
                            }
                        }
                        if (changed) break;
                    }
                }
            }
        }
    }
    
    // Comptage des contours finaux
    long total = 0, edgec = 0;
    for (long i = nrl + 1; i <= nrh - 1; i++) {
        for (long j = ncl + 1; j <= nch - 1; j++) {
            edgec += edges[i][j];
            total++;
        }
    }
    
    // Libération mémoire
    free_imatrix(ix, nrl, nrh, ncl, nch);
    free_imatrix(iy, nrl, nrh, ncl, nch);
    free_dmatrix(mag_suppressed, nrl, nrh, ncl, nch);
    free_bmatrix(strong_edges, nrl, nrh, ncl, nch);
    free_bmatrix(weak_edges, nrl, nrh, ncl, nch);
    
    // Densité
    return (total > 0) ? ((double)edgec / (double)total) : 0.0;
}

//calcule histogramme non normalisé
void histogramme256(byte** gray, long nrl ,long nrh, long ncl , long nch , double hist[256]){
    uint64_t H[256] = {0}; // choix uint64 car la somme des pixels peut dépasser le byte
    uint64_t N = 0;
    //initialisation 
    for (int k = 0; k < 256; k++) hist[k] = 0.0;
    //remplissage valeur par valeur 
    for (long i = nrl; i <= nrh; i++) {
        for (long j = ncl; j <= nch; j++) {
            H[gray[i][j]]++;
            N++;
        }
    }
    if (N == 0) return;

}

// Calcule l'histogramme normalisé
void histogramme256_normalise(byte **gray, long nrl, long nrh, long ncl, long nch, double hist[256]) {
    uint64_t H[256] = {0}; // choix uint64 car la somme des pixels peut dépasser le byte
    uint64_t N = 0;
    //initialisation 
    for (int k = 0; k < 256; k++) hist[k] = 0.0;
    //remplissage valeur par valeur 
    for (long i = nrl; i <= nrh; i++) {
        for (long j = ncl; j <= nch; j++) {
            H[gray[i][j]]++;
            N++;
        }
    }
    if (N == 0) return;

    //normalisation
    for (int b = 0; b < 256; b++) {
        hist[b] = (double)H[b] / (double)N;
    }
}


int verifier_image_couleur_est_nb(uint64_t rsum, uint64_t gsum, uint64_t bsum,
                                 long nrl, long nrh, long ncl, long nch) {
    double S = (double)rsum + (double)gsum + (double)bsum;
    if (S <= 0) return 0;
    double r = rsum / S, g = gsum / S, b = bsum / S;
    double maxc = fmax(r, fmax(g, b));
    double minc = fmin(r, fmin(g, b));
    //cas couleur ou noir et blanc
    return ((maxc - minc) < 0.02) ? 0 : 1;
}

//extraction complète des features ici => 
int extraire_features_from_file(const char *filename, ImageFeatures *feat,
                               int do_apply_filtre, double seuil_contour, int image_type) {
    //TODO, à ajouter une fonction qui initialise toutes les ressources dont nous avons besoin 
    memset(feat, 0, sizeof(*feat));

    long nrl, nrh, ncl, nch;
    byte **gray = NULL;
    double r_ratio = 1.0 / 3.0, g_ratio = 1.0 / 3.0, b_ratio = 1.0 / 3.0;
    int is_color = 0;

    if (image_type == IMAGE_TYPE_PPM) {
        rgb8 **rgb = load_ppm_rgb_and_to_gray(filename, &nrl, &nrh, &ncl, &nch, &gray, &r_ratio, &g_ratio, &b_ratio, &is_color);
        if (!rgb) return -1;
        free_rgb8matrix(rgb, nrl, nrh, ncl, nch);
    } else if (image_type == IMAGE_TYPE_PGM) {
        gray = load_pgm_gray(filename, &nrl, &nrh, &ncl, &nch);
        if (!gray) return -1;
        is_color = 0;
    } else if (image_type == IMAGE_TYPE_JPEG) {
        goto je_sais_pas ;  // faudra voir comment on gère ce cas de figure ... 
    } else {
        return -1;  // pareil 
    }
    je_sais_pas : // à enlever cette partie 
    // Remplit les champs de base
    feat->nrl = nrl; feat->nrh = nrh; feat->ncl = ncl; feat->nch = nch;
    feat->width = nch - ncl + 1;
    feat->height = nrh - nrl + 1;
    feat->est_couleur = is_color;
    feat->ratio_rouge = r_ratio;
    feat->ratio_vert = g_ratio;
    feat->ratio_bleu = b_ratio;

    // Filtre moyenne si on l'indique dans param
    if (do_apply_filtre) {
        byte **filt = bmatrix(nrl, nrh, ncl, nch);
        filtre_moyenneur(gray, filt, nrl, nrh, ncl, nch);
        free_bmatrix(gray, nrl, nrh, ncl, nch);
        gray = filt;
    }

    //gradient
    double **mag_norm = (double**)dmatrix(nrl, nrh, ncl, nch);
    feat->moyenne_gradient_norme = gradient_magnitude_norm(gray, mag_norm, nrl, nrh, ncl, nch);

    //contour
    byte **edges = bmatrix(nrl, nrh, ncl, nch);
    feat->densite_contours = detection_contours_hysterisis(mag_norm, edges, nrl, nrh, ncl, nch, seuil_contour);

    //histogramme normalisé et remplissage tabeau directement passé par adresse 
    histogramme256_normalise(gray, nrl, nrh, ncl, nch, feat->hist);
    
    // Calcul des nouveaux moments statistiques
    feat->hist_variance = calculer_variance_histogramme(feat->hist);
    feat->hist_skewness = calculer_skewness_histogramme(feat->hist);
    feat->hist_kurtosis = calculer_kurtosis_histogramme(feat->hist);
    feat->hist_entropy = calculer_entropie_histogramme(feat->hist);
    
    // Calcul des histogrammes multi-échelles (quadrants)
    calculer_histogrammes_quadrants(gray, nrl, nrh, ncl, nch,
                                  feat->hist_quad1, feat->hist_quad2,
                                  feat->hist_quad3, feat->hist_quad4);
    
    // Calcul des caractéristiques de texture améliorées
    feat->gradient_variance = calculer_variance_gradients(gray, nrl, nrh, ncl, nch);
    feat->contour_coherence = calculer_coherence_contours(mag_norm, edges, nrl, nrh, ncl, nch);

    //libération de ressources 
    free_bmatrix(gray, nrl, nrh, ncl, nch);
    free_dmatrix((double**)mag_norm, nrl, nrh, ncl, nch); 
    free_bmatrix(edges, nrl, nrh, ncl, nch);

    return 0;
}

//écrit entête csv
void ecrire_csv_header(FILE *fout) {
    fprintf(fout, "name,width,height,moyenne_gradient_norme,densite_contours,ratio_rouge,ratio_vert,ratio_bleu,is_color");
    for (int i = 0; i < 256; i++) fprintf(fout, ",hist%d", i); //histogramme
    fprintf(fout, "\n");
}

// Écrit une ligne CSV
void ecrire_csv_ligne(FILE *fout, const char *name, const ImageFeatures *feat) {
    fprintf(fout, "%s,%ld,%ld,%f,%f,%f,%f,%f,%d",
            name, feat->width, feat->height,
            feat->moyenne_gradient_norme, feat->densite_contours,
            feat->ratio_rouge, feat->ratio_vert, feat->ratio_bleu,
            feat->est_couleur);
    for (int i = 0; i < 256; i++) fprintf(fout, ",%f", feat->hist[i]);
    fprintf(fout, "\n");
}

double distance_euclidienne(const double hist1[256] , const double hist2[256]) {
    //calcul de la distance euclidienne
    double somme = 0.0 ;
    for(int i=0 ; i<256 ; i++) {
        double difference = hist1[i] - hist2[i];
        double puissance = difference*difference;
        somme += puissance;
    }
    return sqrt(somme);
}



double distance_bhattacharyya(const double hist1[256] ,const double hist2[256]) {
    //calcul de la distance bhatchattarya 
    double sum = 0.0;
    double epsilon = 1e-10;
    for (int i = 0; i < 256; i++) {
        sum += sqrt(hist1[i] * hist2[i]);
    }
    return -log(sum + epsilon); 
}


double distance_hellinger(const double hist1[256], const double hist2[256]) {
    double sum = 0.0;
    for (int i = 0; i < 256; i++) {
        double diff = sqrt(hist1[i]) - sqrt(hist2[i]);
        sum += diff * diff;
    }
    return sqrt(sum) / sqrt(2.0);
}

// Distance du chi-carré
double distance_chi_square(const double hist1[256], const double hist2[256]) {
    double sum = 0.0;
    for (int i = 0; i < 256; i++) {
        double denom = hist1[i] + hist2[i];
        if (denom > 0) {
            double diff = hist1[i] - hist2[i];
            sum += (diff * diff) / denom;
        }
    }
    return sum;
}

// Distance Earth Mover's (Wasserstein) approximée
double distance_earth_mover(const double hist1[256], const double hist2[256]) {
    double distance = 0.0;
    double cumulative1 = 0.0, cumulative2 = 0.0;
    
    for (int i = 0; i < 256; i++) {
        cumulative1 += hist1[i];
        cumulative2 += hist2[i];
        distance += fabs(cumulative1 - cumulative2);
    }
    
    return distance;
}

// Calcul de la variance de l'histogramme
double calculer_variance_histogramme(const double hist[256]) {
    // Calcul de la moyenne
    double mean = 0.0;
    for (int i = 0; i < 256; i++) {
        mean += i * hist[i];
    }
    
    // Calcul de la variance
    double variance = 0.0;
    for (int i = 0; i < 256; i++) {
        double diff = i - mean;
        variance += diff * diff * hist[i];
    }
    
    return variance;
}

// Calcul de l'asymétrie (skewness) de l'histogramme  
double calculer_skewness_histogramme(const double hist[256]) {
    double mean = 0.0, variance = 0.0, skewness = 0.0;
    
    // Moyenne
    for (int i = 0; i < 256; i++) {
        mean += i * hist[i];
    }
    
    // Variance
    for (int i = 0; i < 256; i++) {
        double diff = i - mean;
        variance += diff * diff * hist[i];
    }
    
    if (variance == 0.0) return 0.0;
    double std_dev = sqrt(variance);
    
    // Skewness
    for (int i = 0; i < 256; i++) {
        double normalized = (i - mean) / std_dev;
        skewness += normalized * normalized * normalized * hist[i];
    }
    
    return skewness;
}

// Calcul de l'aplatissement (kurtosis) de l'histogramme
double calculer_kurtosis_histogramme(const double hist[256]) {
    double mean = 0.0, variance = 0.0, kurtosis = 0.0;
    
    // Moyenne
    for (int i = 0; i < 256; i++) {
        mean += i * hist[i];
    }
    
    // Variance  
    for (int i = 0; i < 256; i++) {
        double diff = i - mean;
        variance += diff * diff * hist[i];
    }
    
    if (variance == 0.0) return 0.0;
    double std_dev = sqrt(variance);
    
    // Kurtosis
    for (int i = 0; i < 256; i++) {
        double normalized = (i - mean) / std_dev;
        double powered = normalized * normalized * normalized * normalized;
        kurtosis += powered * hist[i];
    }
    
    return kurtosis - 3.0; // Excès de kurtosis (kurtosis normale = 3)
}

// Calcul de l'entropie de l'histogramme
double calculer_entropie_histogramme(const double hist[256]) {
    double entropy = 0.0;
    const double epsilon = 1e-10;
    
    for (int i = 0; i < 256; i++) {
        if (hist[i] > epsilon) {
            entropy -= hist[i] * log2(hist[i]);
        }
    }
    
    return entropy;
}

// Calcul des histogrammes pour les 4 quadrants de l'image
void calculer_histogrammes_quadrants(byte **gray, long nrl, long nrh, long ncl, long nch,
                                   double hist_q1[256], double hist_q2[256], 
                                   double hist_q3[256], double hist_q4[256]) {
    // Initialisation des histogrammes
    for (int k = 0; k < 256; k++) {
        hist_q1[k] = hist_q2[k] = hist_q3[k] = hist_q4[k] = 0.0;
    }
    
    // Calcul des limites des quadrants
    long mid_row = (nrl + nrh) / 2;
    long mid_col = (ncl + nch) / 2;
    
    uint64_t count_q1 = 0, count_q2 = 0, count_q3 = 0, count_q4 = 0;
    uint64_t H_q1[256] = {0}, H_q2[256] = {0}, H_q3[256] = {0}, H_q4[256] = {0};
    
    // Parcours de l'image et attribution aux quadrants
    for (long i = nrl; i <= nrh; i++) {
        for (long j = ncl; j <= nch; j++) {
            byte pixel = gray[i][j];
            
            if (i <= mid_row && j <= mid_col) {
                // Quadrant 1: haut-gauche
                H_q1[pixel]++;
                count_q1++;
            } else if (i <= mid_row && j > mid_col) {
                // Quadrant 2: haut-droite
                H_q2[pixel]++;
                count_q2++;
            } else if (i > mid_row && j <= mid_col) {
                // Quadrant 3: bas-gauche
                H_q3[pixel]++;
                count_q3++;
            } else {
                // Quadrant 4: bas-droite
                H_q4[pixel]++;
                count_q4++;
            }
        }
    }
    
    // Normalisation des histogrammes
    for (int b = 0; b < 256; b++) {
        if (count_q1 > 0) hist_q1[b] = (double)H_q1[b] / (double)count_q1;
        if (count_q2 > 0) hist_q2[b] = (double)H_q2[b] / (double)count_q2;
        if (count_q3 > 0) hist_q3[b] = (double)H_q3[b] / (double)count_q3;
        if (count_q4 > 0) hist_q4[b] = (double)H_q4[b] / (double)count_q4;
    }
}

// Calcul de la variance des gradients pour mesurer la texture
double calculer_variance_gradients(byte **gray, long nrl, long nrh, long ncl, long nch) {
    int **ix = imatrix(nrl, nrh, ncl, nch);
    int **iy = imatrix(nrl, nrh, ncl, nch);
    sobel_ix_iy(gray, ix, iy, nrl, nrh, ncl, nch);
    
    double sum_mag = 0.0, sum_mag_sq = 0.0;
    long count = 0;
    
    // Calcul des magnitudes et accumulation
    for (long i = nrl + 1; i <= nrh - 1; i++) {
        for (long j = ncl + 1; j <= nch - 1; j++) {
            double gx = (double)ix[i][j];
            double gy = (double)iy[i][j];
            double mag = sqrt(gx * gx + gy * gy);
            
            sum_mag += mag;
            sum_mag_sq += mag * mag;
            count++;
        }
    }
    
    free_imatrix(ix, nrl, nrh, ncl, nch);
    free_imatrix(iy, nrl, nrh, ncl, nch);
    
    if (count == 0) return 0.0;
    
    double mean = sum_mag / (double)count;
    double variance = (sum_mag_sq / (double)count) - (mean * mean);
    
    return variance;
}

// Calcul de la cohérence des contours (mesure la régularité des contours)
double calculer_coherence_contours(double **mag_norm, byte **edges, 
                                 long nrl, long nrh, long ncl, long nch) {
    double coherence_sum = 0.0;
    long edge_count = 0;
    
    // Parcours des pixels de contour pour mesurer la continuité locale
    for (long i = nrl + 1; i <= nrh - 1; i++) {
        for (long j = ncl + 1; j <= nch - 1; j++) {
            if (edges[i][j] == 1) {
                // Compte les voisins qui sont aussi des contours
                int neighbors = 0;
                for (int di = -1; di <= 1; di++) {
                    for (int dj = -1; dj <= 1; dj++) {
                        if (di == 0 && dj == 0) continue;
                        if (edges[i + di][j + dj] == 1) neighbors++;
                    }
                }
                
                // La cohérence est proportionnelle au nombre de voisins contours
                coherence_sum += (double)neighbors / 8.0; // Normalisation par 8 voisins max
                edge_count++;
            }
        }
    }
    
    return (edge_count > 0) ? (coherence_sum / (double)edge_count) : 0.0;
}

// Calcul des poids adaptatifs basés sur le contenu de l'image de référence
AdaptiveWeights calculer_poids_adaptatifs(const ImageFeatures *feat_ref) {
    AdaptiveWeights weights;
    
    // Analyse du contenu pour adapter les poids
    double texture_factor = feat_ref->gradient_variance / 1000.0; // Normalisation approximative
    if (texture_factor > 1.0) texture_factor = 1.0;
    
    double color_importance = feat_ref->est_couleur ? 1.0 : 0.5;
    double contrast_factor = feat_ref->hist_variance / 10000.0; // Normalisation approximative  
    if (contrast_factor > 1.0) contrast_factor = 1.0;
    
    double edge_density = feat_ref->densite_contours;
    
    // Attribution des poids selon le type de contenu détecté
    if (texture_factor > 0.6) {
        // Image très texturée -> privilégier gradients et texture
        weights.weight_hist_global = 0.25;
        weights.weight_hist_local = 0.30;
        weights.weight_moments = 0.15;
        weights.weight_norm = 0.20;
        weights.weight_contour = 0.25;
        weights.weight_texture = 0.30;
    } else if (edge_density > 0.15) {
        // Image avec beaucoup de contours -> privilégier contours
        weights.weight_hist_global = 0.30;
        weights.weight_hist_local = 0.25;
        weights.weight_moments = 0.10;
        weights.weight_norm = 0.15;
        weights.weight_contour = 0.35;
        weights.weight_texture = 0.15;
    } else if (contrast_factor < 0.3) {
        // Image peu contrastée -> privilégier histogrammes fins
        weights.weight_hist_global = 0.40;
        weights.weight_hist_local = 0.35;
        weights.weight_moments = 0.25;
        weights.weight_norm = 0.10;
        weights.weight_contour = 0.10;
        weights.weight_texture = 0.10;
    } else {
        // Configuration équilibrée par défaut
        weights.weight_hist_global = 0.35;
        weights.weight_hist_local = 0.25;
        weights.weight_moments = 0.15;
        weights.weight_norm = 0.15;
        weights.weight_contour = 0.20;
        weights.weight_texture = 0.15;
    }
    
    // Poids couleur adaptatifs
    weights.weight_r = color_importance * 0.04;
    weights.weight_g = color_importance * 0.04;
    weights.weight_b = color_importance * 0.04;
    weights.weight_color = feat_ref->est_couleur ? 0.08 : 0.02;
    
    return weights;
}

//fonctiobn d'évaluation de score ...
double evaluate_score(const ImageFeatures *feat1, const ImageFeatures *feat2, DistanceFunc dist_func,
                      double weight_hist, double weight_r, double weight_g, double weight_b,
                      double weight_norm, double weight_contour, double weight_color) {
    // Distance de l'histogramme
    double dist_hist = dist_func(feat1->hist, feat2->hist);
    // printf("Distance histogramme: %.4f\n", dist_hist);
    
    // Différences absolues pour les ratios RGB
    double diff_r = fabs(feat1->ratio_rouge - feat2->ratio_rouge);
    // printf("Différence ratio rouge: %.4f\n", diff_r);
    double diff_g = fabs(feat1->ratio_vert - feat2->ratio_vert);
    // printf("Différence ratio vert: %.4f\n", diff_g);
    double diff_b = fabs(feat1->ratio_bleu - feat2->ratio_bleu);
    // printf("Différence ratio bleu: %.4f\n", diff_b);
    
    // Différences pour la norme du gradient et la densité des contours
    double diff_norm = fabs(feat1->moyenne_gradient_norme - feat2->moyenne_gradient_norme);
    printf("Différence norme gradient: %.4f\n", diff_norm);
    double diff_contour = fabs(feat1->densite_contours - feat2->densite_contours);
    printf("Différence densité contours: %.4f\n", diff_contour);
    
    //pénalité selon les couleurs => similarité interformat ... 
    double diff_color = (feat1->est_couleur != feat2->est_couleur) ? 1.0 : 0.0;
    printf("Pénalité couleur: %.4f\n", diff_color);
    
    
    // Score pondéré (somme des distances pondérées)
    double score = weight_hist * dist_hist +
                   weight_r * diff_r +
                   weight_g * diff_g +
                   weight_b * diff_b +
                   weight_norm * diff_norm +
                   weight_contour * diff_contour +
                   weight_color * diff_color;
    printf("Score total: %.4f\n", score);
    
    return score;
}

// Fonction d'évaluation du score améliorée avec poids adaptatifs
double evaluate_score_enhanced(const ImageFeatures *feat1, const ImageFeatures *feat2, 
                             DistanceFunc dist_func, const AdaptiveWeights *weights) {
    
    // Distance histogramme global
    double dist_hist_global = dist_func(feat1->hist, feat2->hist);
    
    // Distance histogrammes locaux (quadrants) avec moyenne pondérée
    double dist_hist_local = 0.25 * (
        dist_func(feat1->hist_quad1, feat2->hist_quad1) +
        dist_func(feat1->hist_quad2, feat2->hist_quad2) +
        dist_func(feat1->hist_quad3, feat2->hist_quad3) +
        dist_func(feat1->hist_quad4, feat2->hist_quad4)
    );
    
    // Distance des moments statistiques
    double diff_variance = fabs(feat1->hist_variance - feat2->hist_variance) / 10000.0;
    double diff_skewness = fabs(feat1->hist_skewness - feat2->hist_skewness) / 2.0;
    double diff_kurtosis = fabs(feat1->hist_kurtosis - feat2->hist_kurtosis) / 5.0;
    double diff_entropy = fabs(feat1->hist_entropy - feat2->hist_entropy) / 8.0;
    
    // Normalisation et combinaison des moments  
    if (diff_variance > 1.0) diff_variance = 1.0;
    if (diff_skewness > 1.0) diff_skewness = 1.0;
    if (diff_kurtosis > 1.0) diff_kurtosis = 1.0;
    if (diff_entropy > 1.0) diff_entropy = 1.0;
    
    double dist_moments = 0.25 * (diff_variance + diff_skewness + diff_kurtosis + diff_entropy);
    
    // Différences couleur
    double diff_r = fabs(feat1->ratio_rouge - feat2->ratio_rouge);
    double diff_g = fabs(feat1->ratio_vert - feat2->ratio_vert);
    double diff_b = fabs(feat1->ratio_bleu - feat2->ratio_bleu);
    
    // Différences texture et contours
    double diff_norm = fabs(feat1->moyenne_gradient_norme - feat2->moyenne_gradient_norme);
    double diff_contour = fabs(feat1->densite_contours - feat2->densite_contours);
    
    // Nouvelles caractéristiques de texture
    double diff_grad_var = fabs(feat1->gradient_variance - feat2->gradient_variance) / 1000.0;
    if (diff_grad_var > 1.0) diff_grad_var = 1.0;
    
    double diff_coherence = fabs(feat1->contour_coherence - feat2->contour_coherence);
    
    double dist_texture = 0.5 * (diff_grad_var + diff_coherence);
    
    // Pénalité couleur
    double diff_color = (feat1->est_couleur != feat2->est_couleur) ? 1.0 : 0.0;
    
    // Score final pondéré et normalisé
    double score = weights->weight_hist_global * dist_hist_global +
                   weights->weight_hist_local * dist_hist_local +
                   weights->weight_moments * dist_moments +
                   weights->weight_r * diff_r +
                   weights->weight_g * diff_g +
                   weights->weight_b * diff_b +
                   weights->weight_norm * diff_norm +
                   weights->weight_contour * diff_contour +
                   weights->weight_texture * dist_texture +
                   weights->weight_color * diff_color;
    
    return score;
}