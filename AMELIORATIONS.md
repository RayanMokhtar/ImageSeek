# ImageSeek - Améliorations Apportées

## Vue d'ensemble
ImageSeek est un système de recherche d'images basé sur des descripteurs personnalisés. Les améliorations apportées visent à améliorer significativement la pertinence du classement des images en enrichissant les caractéristiques extraites et en optimisant les méthodes de comparaison.

## Améliorations Implémentées

### 1. **Enrichissement des Caractéristiques d'Images**

#### A. Moments Statistiques de l'Histogramme
**Avant :** Seul l'histogramme normalisé était utilisé.

**Après :** Ajout de 4 nouveaux moments statistiques :
- **Variance** : Mesure la dispersion des intensités
- **Asymétrie (Skewness)** : Indique si la distribution est décalée vers les tons clairs/sombres
- **Aplatissement (Kurtosis)** : Caractérise la forme de la distribution (pics nets vs distributions plates)
- **Entropie** : Quantifie la richesse informationnelle de l'image

**Impact :** Ces métriques permettent de distinguer finement des images ayant des histogrammes similaires mais des distributions différentes.

#### B. Histogrammes Multi-Échelles (Quadrants)
**Avant :** Un seul histogramme global.

**Après :** 4 histogrammes locaux (quadrants) + l'histogramme global.
- Quadrant haut-gauche, haut-droite, bas-gauche, bas-droite
- Capture la distribution spatiale des intensités

**Impact :** Permet de différencier des images ayant la même distribution globale mais une répartition spatiale différente (ex: ciel en haut vs en bas).

### 2. **Détection de Contours Améliorée**

#### A. Suppression de Non-Maxima
**Avant :** Simple seuillage de la magnitude des gradients.

**Après :** Implémentation de la suppression de non-maxima :
- Calcul de la direction du gradient (0°, 45°, 90°, 135°)
- Conservation uniquement des maxima locaux dans la direction du gradient
- Contours plus fins et plus précis

#### B. Hystérésis à Double Seuil
**Avant :** Seuil unique fixe.

**Après :** Hystérésis avec seuils haut et bas :
- Seuil haut : contours forts (sûrs)
- Seuil bas : contours faibles (candidats)
- Connexion des contours faibles aux contours forts
- Réduction du bruit tout en préservant la continuité

**Impact :** Détection de contours plus robuste et moins sensible au bruit.

### 3. **Analyse de Texture Avancée**

#### A. Variance des Gradients
**Avant :** Seule la moyenne des gradients était calculée.

**Après :** Calcul de la variance des magnitudes de gradients.
- Mesure la régularité/irrégularité des variations d'intensité
- Distingue les textures lisses des textures rugueuses

#### B. Cohérence des Contours
**Nouveau :** Mesure de la continuité locale des contours.
- Pour chaque pixel de contour, compte les voisins également contours
- Évalue la régularité des structures dans l'image

**Impact :** Meilleure caractérisation de la complexité structurelle des images.

### 4. **Distances de Comparaison Enrichies**

#### A. Distance Earth Mover's (Wasserstein)
**Avant :** Distances classiques (Euclidienne, Bhattacharyya, Chi-carré, Hellinger).

**Après :** Ajout de la distance Earth Mover's :
- Mesure le "coût de transport" entre deux distributions
- Plus sensible aux translations dans l'histogramme
- Meilleure pour comparer des images avec des décalages d'intensité

**Impact :** Comparaisons d'histogrammes plus robustes aux variations d'éclairage.

### 5. **Système de Pondération Adaptatif**

#### A. Analyse du Contenu de Référence
**Avant :** Poids fixes identiques pour toutes les images.

**Après :** Poids calculés automatiquement selon le contenu :

**Images très texturées** (variance gradients > 0.6) :
- Privilégie les caractéristiques de texture (30%)
- Histogrammes locaux renforcés (30%)
- Contours modérés (25%)

**Images riches en contours** (densité > 0.15) :
- Emphasise la détection de contours (35%)
- Caractéristiques de forme prioritaires
- Texture secondaire (15%)

**Images peu contrastées** (variance < 0.3) :
- Histogrammes fins privilégiés (40% global + 35% local)
- Moments statistiques renforcés (25%)
- Contours/texture minimisés

**Images équilibrées** (cas par défaut) :
- Distribution équilibrée entre toutes les caractéristiques

**Impact :** Adaptation automatique de la stratégie de comparaison au type d'image analysée.

### 6. **Fonction de Score Améliorée**

#### A. Normalisation Intelligente
**Avant :** Scores bruts non normalisés.

**Après :** Normalisation adaptée à chaque type de caractéristique :
- Moments statistiques normalisés par leurs valeurs typiques
- Distances pondérées selon leur importance relative
- Évitement de la domination d'une caractéristique sur les autres

#### B. Agrégation Multi-Niveau
**Nouveau :** Combinaison hiérarchique :
1. Distances d'histogrammes (global + 4 locaux)
2. Moments statistiques (4 métriques combinées)
3. Caractéristiques de texture (2 métriques)
4. Agrégation finale pondérée selon le contenu

**Impact :** Score final plus équilibré et représentatif de la similarité globale.

## Résultats et Performances

### Améliorations Observées

1. **Précision du Classement** : 
   - Meilleure discrimination entre images similaires
   - Réduction des faux positifs dans les résultats

2. **Robustesse** :
   - Moins sensible aux variations d'éclairage (Earth Mover's)
   - Meilleure gestion du bruit (hystérésis)

3. **Adaptabilité** :
   - Configuration automatique selon le type d'image
   - Performances consistantes sur différents contenus

### Exemple d'Utilisation

```c
// Configuration automatique basée sur l'image de référence
AdaptiveWeights weights = calculer_poids_adaptatifs(&feat_ref);

// Utilisation de la nouvelle fonction de comparaison
double score = evaluate_score_enhanced(&feat_ref, &feat_curr, 
                                     distance_earth_mover, &weights);
```

### Configuration Adaptative Exemple

Pour l'image de référence `arbre1.pgm` :
- **Type détecté** : Image texturée (variance gradients = 11203)
- **Poids histogramme global** : 0.25
- **Poids histogramme local** : 0.30  
- **Poids moments** : 0.15
- **Poids texture** : 0.30
- **Poids contours** : 0.25

## Architecture Technique

### Nouvelles Structures de Données

```c
typedef struct {
    // Caractéristiques originales
    double hist[256];
    double moyenne_gradient_norme;
    double densite_contours;
    // ... autres champs existants
    
    // Nouveaux moments statistiques
    double hist_variance;
    double hist_skewness; 
    double hist_kurtosis;
    double hist_entropy;
    
    // Histogrammes multi-échelles
    double hist_quad1[256], hist_quad2[256];
    double hist_quad3[256], hist_quad4[256];
    
    // Texture améliorée
    double gradient_variance;
    double contour_coherence;
} ImageFeatures;

typedef struct {
    double weight_hist_global;
    double weight_hist_local;
    double weight_moments;
    double weight_texture;
    // ... autres poids
} AdaptiveWeights;
```

### Nouvelles Fonctions Clés

- `calculer_variance_histogramme()` : Moments statistiques
- `calculer_histogrammes_quadrants()` : Distribution spatiale
- `suppression_non_maxima()` : Affinage des contours
- `calculer_poids_adaptatifs()` : Configuration automatique
- `evaluate_score_enhanced()` : Score multi-critères

## Conclusion

Ces améliorations transforment ImageSeek d'un système de comparaison basique à un moteur de recherche d'images sophistiqué et adaptatif. L'approche multi-caractéristiques avec pondération automatique permet une pertinence de recherche significativement améliorée tout en conservant la simplicité d'utilisation du système original.

Les nouvelles fonctionnalités restent compatibles avec l'API existante, permettant une migration progressive des applications utilisant ImageSeek.
