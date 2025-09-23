# ImageSeek - Système de Recherche d'Images Avancé

## Description
ImageSeek est un système de recherche d'images sophistiqué utilisant des descripteurs personnalisés multi-niveau pour une comparaison et un classement précis des images. Le système a été considérablement amélioré avec des techniques avancées de vision par ordinateur.

## Fonctionnalités Principales

### Caractéristiques d'Images Extraites
- **Histogrammes multi-échelles** : Global + 4 quadrants spatiaux
- **Moments statistiques** : Variance, asymétrie, aplatissement, entropie
- **Détection de contours avancée** : Suppression de non-maxima + hystérésis
- **Analyse de texture** : Variance des gradients, cohérence des contours
- **Ratios colorimétriques** : RGB avec détection automatique couleur/N&B

### Méthodes de Comparaison
- **Distances d'histogrammes** : Euclidienne, Bhattacharyya, Hellinger, Chi-carré, Earth Mover's
- **Pondération adaptative** : Configuration automatique selon le type d'image
- **Score multi-critères** : Agrégation intelligente de toutes les caractéristiques

### Système Adaptatif
Le système adapte automatiquement ses paramètres selon le contenu de l'image de référence :
- **Images texturées** : Privilégie les caractéristiques de texture
- **Images à contours** : Met l'accent sur la détection de formes
- **Images peu contrastées** : Utilise des analyses d'histogrammes fins
- **Images équilibrées** : Configuration optimisée généraliste

## Compilation et Utilisation

### Compilation
```bash
make                    # Compile le programme principal
make comparison         # Compile la démonstration comparative
make clean              # Nettoie les fichiers compilés
```

### Exécution
```bash
./main_programme.exe     # Lance l'analyse avec image de référence
./comparison_demo.exe    # Compare ancien vs nouveau système
```

### Exemple d'Utilisation dans le Code

```c
#include "image.h"

int main() {
    ImageFeatures feat_ref;
    
    // Extraction des caractéristiques avancées
    extraire_features_from_file("reference.pgm", &feat_ref, 0, 0.25, IMAGE_TYPE_PGM);
    
    // Configuration adaptative automatique
    AdaptiveWeights weights = calculer_poids_adaptatifs(&feat_ref);
    
    // Comparaison avec score amélioré
    ImageFeatures feat_test;
    extraire_features_from_file("test.pgm", &feat_test, 0, 0.25, IMAGE_TYPE_PGM);
    
    double score = evaluate_score_enhanced(&feat_ref, &feat_test, 
                                         distance_earth_mover, &weights);
    
    printf("Score de similarité : %.4f\n", score);
    return 0;
}
```

## Architecture Technique

### Structure des Données
- `ImageFeatures` : Structure enrichie avec 15+ caractéristiques
- `AdaptiveWeights` : Système de pondération adaptative
- Support des formats PGM (niveaux de gris) et PPM (couleur)

### Algorithmes Implémentés
- **Gradients Sobel** avec suppression de non-maxima
- **Hystérésis à double seuil** pour contours robustes
- **Earth Mover's Distance** pour comparaison d'histogrammes
- **Analyse statistique** multi-moments des distributions
- **Segmentation spatiale** en quadrants pour contexte géométrique

## Améliorations par Rapport à la Version Originale

### Précision
- ✅ Discrimination fine entre images similaires
- ✅ Réduction des faux positifs
- ✅ Meilleure gestion des variations d'éclairage

### Robustesse  
- ✅ Moins sensible au bruit (hystérésis)
- ✅ Adaptation automatique au contenu
- ✅ Normalisation intelligente des scores

### Performance
- ✅ Configuration optimisée par type d'image
- ✅ Utilisation efficace de multiples caractéristiques
- ✅ Compatibilité avec l'API existante

## Documentation Complète

Consultez [AMELIORATIONS.md](AMELIORATIONS.md) pour une analyse détaillée de toutes les améliorations techniques implémentées.

## Structure du Projet

```
ImageSeek/
├── main.c              # Programme principal amélioré
├── comparison_demo.c   # Démonstration comparative
├── image.c/.h          # Moteur de traitement d'images
├── makefile           # Configuration de compilation
├── nrc/              # Bibliothèque NRC pour I/O images
├── archivePPMPGM/    # Images de test
└── docs/             # Documentation
```

## Formats Supportés
- **PGM** : Images en niveaux de gris
- **PPM** : Images couleur (converties automatiquement si nécessaire)

## Performance

Le système traite efficacement des images de tailles variées et s'adapte automatiquement pour optimiser la pertinence selon le type de contenu analysé. La comparaison multi-critères avec pondération adaptative offre des résultats de recherche significativement plus précis que les approches traditionnelles basées sur un seul type de descripteur.
