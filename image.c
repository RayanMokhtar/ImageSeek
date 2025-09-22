#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "image.h"

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
    // Alloue la matrice pour la version en gris
    *pgray = bmatrix(*nrl, *nrh, *ncl, *nch);

    // Sommes des canaux pour calculer les ratios
    uint64_t Rsum = 0, Gsum = 0, Bsum = 0;

    // Parcours tous les pixels
    for (i = *nrl; i <= *nrh; i++) {
        for (j = *ncl; j <= *nch; j++) {
            // Accumule les sommes
            Rsum += rgb[i][j].r;
            Gsum += rgb[i][j].g;
            Bsum += rgb[i][j].b;

            // Conversion en gris : formule luminance standard
            double y = 0.299 * rgb[i][j].r + 0.587 * rgb[i][j].g + 0.114 * rgb[i][j].b;
            (*pgray)[i][j] = clampi((int)(y + 0.5));  // Arrondi et clamp
        }
    }

    // Calcule les ratios normalisés
    double S = (double)Rsum + (double)Gsum + (double)Bsum;
    if (S <= 0.0) {
        *r_ratio = *g_ratio = *b_ratio = 1.0 / 3.0;  // Cas dégénéré
    } else {
        *r_ratio = Rsum / S;
        *g_ratio = Gsum / S;
        *b_ratio = Bsum / S;
    }

    // Détecte si couleur
    *is_color = guess_is_color_from_rgb_sums(Rsum, Gsum, Bsum, *nrl, *nrh, *ncl, *nch);

    return rgb;
}

// Sauvegarde une matrice en PGM
void save_pgm_gray(const char *filename, byte **m, long nrl, long nrh, long ncl, long nch) {
    SavePGM_bmatrix(m, nrl, nrh, ncl, nch, (char*)filename);
}

// Applique un filtre moyenneur 3x3 : convolution avec masque uniforme (1/9 partout)
void filtre_moyenneur(byte **in, byte **out, long nrl, long nrh, long ncl, long nch) {
    long i, j;
    // Traite l'intérieur (évite bords de 1 pixel)
    for (i = nrl + 1; i <= nrh - 1; i++) {
        for (j = ncl + 1; j <= nch - 1; j++) {
            // Somme des 9 voisins
            int s = 0;
            s += in[i-1][j-1] + in[i-1][j] + in[i-1][j+1];
            s += in[i  ][j-1] + in[i  ][j] + in[i  ][j+1];
            s += in[i+1][j-1] + in[i+1][j] + in[i+1][j+1];
            // Moyenne entière
            out[i][j] = (byte)(s / 9);
        }
    }
    // Bords : copie simple de l'entrée
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
    // Masques Sobel
    static const int SX[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};  // Horizontal
    static const int SY[3][3] = {{-1, -2, -1}, {0, 0, 0}, {1, 2, 1}};  // Vertical

    long i, j;
    // Traite l'intérieur
    for (i = nrl + 1; i <= nrh - 1; i++) {
        for (j = ncl + 1; j <= nch - 1; j++) {
            int gx = 0, gy = 0;
            // Convolution avec SX pour gx
            gx += SX[0][0] * gray[i-1][j-1] + SX[0][1] * gray[i-1][j] + SX[0][2] * gray[i-1][j+1];
            gx += SX[1][0] * gray[i  ][j-1] + SX[1][1] * gray[i  ][j] + SX[1][2] * gray[i  ][j+1];
            gx += SX[2][0] * gray[i+1][j-1] + SX[2][1] * gray[i+1][j] + SX[2][2] * gray[i+1][j+1];
            // Convolution avec SY pour gy
            gy += SY[0][0] * gray[i-1][j-1] + SY[0][1] * gray[i-1][j] + SY[0][2] * gray[i-1][j+1];
            gy += SY[1][0] * gray[i  ][j-1] + SY[1][1] * gray[i  ][j] + SY[1][2] * gray[i  ][j+1];
            gy += SY[2][0] * gray[i+1][j-1] + SY[2][1] * gray[i+1][j] + SY[2][2] * gray[i+1][j+1];
            ix[i][j] = gx;
            iy[i][j] = gy;
        }
    }
    // Bords à 0
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
    // Alloue Ix et Iy
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

// Crée la carte des contours et retourne la densité
double detection_contours_hysterisis(double **mag_norm, byte **edges,
                                     long nrl, long nrh, long ncl, long nch, double t_norm) {
    long total = 0, edgec = 0;
    // Traite l'intérieur
    for (long i = nrl + 1; i <= nrh - 1; i++) {
        for (long j = ncl + 1; j <= nch - 1; j++) {
            byte e = (mag_norm[i][j] >= t_norm) ? 1 : 0;
            edges[i][j] = e;
            edgec += e;
            total++;
        }
    }
    // Bords à 0
    for (long i = nrl; i <= nrh; i++) {
        edges[i][ncl] = edges[i][nch] = 0;
    }
    for (long j = ncl; j <= nch; j++) {
        edges[nrl][j] = edges[nrh][j] = 0;
    }
    // Densité
    return (total > 0) ? ((double)edgec / (double)total) : 0.0;
}

// Calcule l'histogramme normalisé
void histogram256_norm(byte **gray, long nrl, long nrh, long ncl, long nch, double hist[256]) {
    uint64_t H[256] = {0};
    uint64_t N = 0;
    // Initialise hist directement sans malloc ici 
    for (int k = 0; k < 256; k++) hist[k] = 0.0;
    //remplissage
    for (long i = nrl; i <= nrh; i++) {
        for (long j = ncl; j <= nch; j++) {
            H[gray[i][j]]++;
            N++;
        }
    }
    if (N == 0) return;

    // Normalise
    for (int b = 0; b < 256; b++) {
        hist[b] = (double)H[b] / (double)N;
    }
}

// Heuristique pour couleur/N&B
int guess_is_color_from_rgb_sums(uint64_t rsum, uint64_t gsum, uint64_t bsum,
                                 long nrl, long nrh, long ncl, long nch) {
    double S = (double)rsum + (double)gsum + (double)bsum;
    if (S <= 0) return 0;
    double r = rsum / S, g = gsum / S, b = bsum / S;
    double maxc = fmax(r, fmax(g, b));
    double minc = fmin(r, fmin(g, b));
    // Si écart < 2%, considère N&B
    return ((maxc - minc) < 0.02) ? 0 : 1;
}

// Extraction complète des features
int extraire_features_from_file(const char *filename, ImageFeatures *feat,
                               int do_apply_filtre, double seuil_contour, int image_type) {
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
    histogram256_norm(gray, nrl, nrh, ncl, nch, feat->hist);


    //libération de ressources 
    free_bmatrix(gray, nrl, nrh, ncl, nch);
    free_dmatrix((double**)mag_norm, nrl, nrh, ncl, nch); 
    free_bmatrix(edges, nrl, nrh, ncl, nch);

    return 0;
}

// Écrit l'en-tête CSV
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