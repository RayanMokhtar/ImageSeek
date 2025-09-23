#ifndef IMAGE_H
#define IMAGE_H

#include <stdio.h>
#include <stdint.h>
#include <math.h>

//ajout des dépendances ici 
#include "nrc/def.h"
#include "nrc/nrio.h"
#include "nrc/nralloc.h"
#include "nrc/nrarith.h"


#define VAL_SOBEL_MAX_THEORIQUE 1500 //à définir par la suite pour faciliter la normalisation
#define SEUIL_CONTOUR 0.25 //à modifier au fil des tests
//constantes pour différencier la logique de traitement
#define IMAGE_TYPE_PGM 0  // Niveaux de gris
#define IMAGE_TYPE_PPM 1  // Couleur
#define IMAGE_TYPE_JPEG 2  // Non supporté par NRC (retourne erreur)


typedef struct {
    long nrl, nrh, ncl, nch; //bornes de l'image
    long width, height;      // W*H => déduire nbr de pixels 
    //Caractéristiques
    double moyenne_gradient_norme;   
    double densite_contours;     
    double ratio_rouge;        
    double ratio_vert;      
    double ratio_bleu;       
    int  est_couleur;         
    double hist[256];       
    
    // Nouvelles caractéristiques améliorées
    double hist_variance;     // Variance de l'histogramme
    double hist_skewness;     // Asymétrie de l'histogramme
    double hist_kurtosis;     // Aplatissement de l'histogramme
    double hist_entropy;      // Entropie de l'histogramme
    
    // Histogrammes multi-échelles (4 quadrants)
    double hist_quad1[256];   // Quadrant haut-gauche
    double hist_quad2[256];   // Quadrant haut-droite  
    double hist_quad3[256];   // Quadrant bas-gauche
    double hist_quad4[256];   // Quadrant bas-droite
    
    // Texture améliorée
    double gradient_variance; // Variance des gradients
    double contour_coherence; // Cohérence des contours
} ImageFeatures;





//elle Charge une image PGM (niveaux de gris) et retourne la matrice byte**
//remplit les bornes NRC (nrl à nrh (tout en bas), ncl, nch ( tout à gauche)) => retour tableau de pointeur byte *[] où chaque elément pointe vers tableau 1D 
byte **load_pgm_gray(const char *filename, long *nrl, long *nrh, long *ncl, long *nch);

//Charge une image PPM (couleur), convertit en gris, calcule les ratios RGB et détecte si couleur
// Retourne la matrice rgb8**, et remplit *pgray avec la version en gris => formule byte gris = (byte)(0.299*r + 0.587*g + 0.114*b)
rgb8 **load_ppm_rgb_and_to_gray(const char *filename,
                                long *nrl, long *nrh, long *ncl, long *nch,
                                byte ***pgray,
                                double *r_ratio, double *g_ratio, double *b_ratio,
                                int *is_color);


//sauvegarde format pgm ...
void save_pgm_gray(const char *filename, byte **m, long nrl, long nrh, long ncl, long nch);

//Applique un filtre moyenneur 3x3 (convolution avec masque uniforme) => évite pas de padding ici
void filtre_moyenneur(byte **in, byte **out, long nrl, long nrh, long ncl, long nch);

// Calcule les gradients Sobel Ix (horizontal) et Iy (vertical) en int toujours sans padding
//entrée matrice en niveaux de gris, 
void sobel_ix_iy(byte **gray, int **ix, int **iy, long nrl, long nrh, long ncl, long nch);

//Calcule la norme du gradient normalisée (0..1) et retourne la moyenne
double gradient_magnitude_norm(byte **gray, double **mag_norm,
                               long nrl, long nrh, long ncl, long nch);

//magnitude gradient normalisée en entrée => retourne les seuils
double detection_contours_hysterisis(double **mag_norm, byte **edges,
                      long nrl, long nrh, long ncl, long nch, double t_norm);

// Calcule l'histogramme des 256 niveaux de gris, normalisé (somme = 1)
void histogramme256_normalise(byte **gray, long nrl, long nrh, long ncl, long nch, double hist[256]);


// Fonction principale : extrait toutes les caractéristiques d'un fichier image (PGM/PPM auto-détecté)
//si apply_filtre à 1 => on fait le filtre moyenneur sinon non
//seuil_contour : seuil pour les contours (par défaut SEUIL_CONTOUR) 
int extraire_features_from_file(const char *filename, ImageFeatures *feat,
                               int do_apply_filtre , double seuil_contour , int image_type); //pointeur ici sur le feature car on veut pas créer de copie

// Écrit l'en-tête CSV (noms des colonnes)
void ecrire_csv_header(FILE *fout);

void ecrire_csv_ligne(FILE *fout, const char *name, const ImageFeatures *feat);



//pointeur de fct ici utile pour choisir la fonction à la volée directement en paramètre
//parenthèses autour de DistanceFunc est la convention pour déclarer un pointeru de fonction
typedef double (*DistanceFunc)(const double hist1[256], const double hist2[256]);


double distance_euclidienne(const double hist1[256], const double hist2[256]);
double distance_bhattacharyya(const double hist1[256], const double hist2[256]);
double distance_hellinger(const double hist1[256], const double hist2[256]);
double distance_chi_square(const double hist1[256], const double hist2[256]);
double distance_earth_mover(const double hist1[256], const double hist2[256]);

// Nouvelles fonctions pour moments statistiques
double calculer_variance_histogramme(const double hist[256]);
double calculer_skewness_histogramme(const double hist[256]);
double calculer_kurtosis_histogramme(const double hist[256]);
double calculer_entropie_histogramme(const double hist[256]);

// Histogrammes multi-échelles
void calculer_histogrammes_quadrants(byte **gray, long nrl, long nrh, long ncl, long nch,
                                   double hist_q1[256], double hist_q2[256], 
                                   double hist_q3[256], double hist_q4[256]);

// Texture améliorée
double calculer_variance_gradients(byte **gray, long nrl, long nrh, long ncl, long nch);
double calculer_coherence_contours(double **mag_norm, byte **edges, 
                                 long nrl, long nrh, long ncl, long nch);

// Fonction utilitaire pour la détection couleur
int verifier_image_couleur_est_nb(uint64_t rsum, uint64_t gsum, uint64_t bsum,
                                 long nrl, long nrh, long ncl, long nch);

// Structure pour les poids adaptatifs
typedef struct {
    double weight_hist_global;
    double weight_hist_local;
    double weight_moments;
    double weight_r, weight_g, weight_b;
    double weight_norm, weight_contour, weight_color;
    double weight_texture;
} AdaptiveWeights;

// Calcul des poids adaptatifs basés sur le contenu
AdaptiveWeights calculer_poids_adaptatifs(const ImageFeatures *feat_ref);

// Fonction d'évaluation du score de similarité (version simple)
double evaluate_score(const ImageFeatures *feat1, const ImageFeatures *feat2, DistanceFunc dist_func,
                      double weight_hist, double weight_r, double weight_g, double weight_b,
                      double weight_norm, double weight_contour, double weight_color);

// Fonction d'évaluation du score de similarité améliorée
double evaluate_score_enhanced(const ImageFeatures *feat1, const ImageFeatures *feat2, 
                             DistanceFunc dist_func, const AdaptiveWeights *weights);


#endif 