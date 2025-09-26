--1. Création du type pour l’histogramme
CREATE TYPE HISTOGRAM256 AS VARRAY(256) OF NUMBER;
/

CREATE TABLE MULTIMEDIA (
  ID            NUMBER PRIMARY KEY,
  NOM           VARCHAR2(200) UNIQUE NOT NULL,
  IMAGE         ORDSYS.ORDIMAGE,
  SIGNATURE     ORDSYS.ORDIMAGESIGNATURE,
  -- Caractéristiques "Partie I"
  RED_RATIO     NUMBER,
  GREEN_RATIO   NUMBER,
  BLUE_RATIO    NUMBER,
  GRADIENT_MEAN NUMBER,
  CONTOUR_COUNT NUMBER,
  HIST_GRAY     HISTOGRAM256
);
/


-- 3. Création de la séquence et du trigger (pour remplacer IDENTITY)

CREATE SEQUENCE MULTIMEDIA_SEQ START WITH 1 INCREMENT BY 1 NOCACHE;
/

CREATE OR REPLACE TRIGGER MULTIMEDIA_BI
BEFORE INSERT ON MULTIMEDIA
FOR EACH ROW
BEGIN
  IF :NEW.ID IS NULL THEN
    SELECT MULTIMEDIA_SEQ.NEXTVAL INTO :NEW.ID FROM dual;
  END IF;
END;
/

--4. Insertion des lignes (images 1 à 500)
BEGIN
  FOR i IN 1..500 LOOP
    INSERT INTO MULTIMEDIA (NOM, IMAGE, SIGNATURE)
    VALUES (TO_CHAR(i) || '.jpg',
            ORDSYS.ORDIMAGE.INIT(),
            ORDSYS.ORDIMAGESIGNATURE.INIT());
  END LOOP;
  COMMIT;
END;
/

---5. Importer les fichiers dans la colonne IMAGE
DECLARE
  ctx RAW(400) := NULL;
  img ORDSYS.ORDIMAGE;
BEGIN
  FOR i IN 1..500 LOOP
    BEGIN
      SELECT IMAGE INTO img
      FROM MULTIMEDIA
      WHERE NOM = TO_CHAR(i) || '.jpg'
      FOR UPDATE;

      img.importFrom(ctx, 'file', 'IMG', TO_CHAR(i) || '.jpg');

      UPDATE MULTIMEDIA
      SET IMAGE = img
      WHERE NOM = TO_CHAR(i) || '.jpg';

    EXCEPTION
      WHEN OTHERS THEN
        DBMS_OUTPUT.PUT_LINE('Problème avec ' || i || '.jpg : ' || SQLERRM);
    END;
  END LOOP;
  COMMIT;
END;
/

--6. Génération des signatures Oracle
DECLARE
  CURSOR c IS SELECT ROWID rid, IMAGE, SIGNATURE, NOM FROM MULTIMEDIA FOR UPDATE;
BEGIN
  FOR r IN c LOOP
    BEGIN
      r.SIGNATURE.generateSignature(r.IMAGE);
      UPDATE MULTIMEDIA
      SET SIGNATURE = r.SIGNATURE
      WHERE ROWID = r.rid;
    EXCEPTION
      WHEN OTHERS THEN
        DBMS_OUTPUT.PUT_LINE('Erreur signature pour ' || r.NOM || ' : ' || SQLERRM);
    END;
  END LOOP;
  COMMIT;
END;
/



--7. recherche d’images similaires (Top-10) pour une image donnée
SET SERVEROUTPUT ON;

DECLARE
  TYPE t_score IS RECORD (
    nom   VARCHAR2(200),
    score NUMBER
  );
  TYPE t_tab IS TABLE OF t_score INDEX BY PLS_INTEGER;

  scores      t_tab;
  refsig      ORDSYS.ORDIMAGESIGNATURE;
  i           PLS_INTEGER := 0;
  tmp         t_score;
  v_image_nom VARCHAR2(200) := '382.jpg';  -- <<< une seule fois ici
BEGIN
  -- Signature de référence
  SELECT signature INTO refsig
  FROM multimedia
  WHERE nom = v_image_nom;

  -- Calculer tous les scores
  FOR rec IN (SELECT nom, signature FROM multimedia WHERE nom <> v_image_nom) LOOP
    i := i + 1;
    scores(i).nom   := rec.nom;
    scores(i).score := ORDSYS.ORDIMAGESIGNATURE.evaluateScore(
                         refsig,
                         rec.signature,
                         'color=0.1,texture=0.4,shape=0.4,location=0.1'
                       );
  END LOOP;

  -- Tri à bulles (simple mais OK pour quelques centaines d’images)
  FOR a IN 1..i LOOP
    FOR b IN a+1..i LOOP
      IF scores(b).score < scores(a).score THEN
        tmp := scores(a);
        scores(a) := scores(b);
        scores(b) := tmp;
      END IF;
    END LOOP;
  END LOOP;

  -- Afficher le Top-10
  DBMS_OUTPUT.PUT_LINE('--- Top 10 similaires à ' || v_image_nom || ' ---');
  FOR k IN 1..LEAST(10,i) LOOP
    DBMS_OUTPUT.PUT_LINE(scores(k).nom || ' => score = ' || scores(k).score);
  END LOOP;
END;
/



--- TABLES FEATURES ... export csv





--------------------------------------------------------------- Script des fonctions 

--faire le parsing

 CREATE OR REPLACE FUNCTION evaluate_score_plsql(
    feat1_nom IN VARCHAR2,
    feat2_nom IN VARCHAR2,
    weight_hist IN NUMBER,
    weight_r IN NUMBER,
    weight_g IN NUMBER,
    weight_b IN NUMBER,
    weight_norm IN NUMBER,
    weight_contour IN NUMBER,
    weight_color IN NUMBER
) RETURN NUMBER IS
    -- Variables for features from multimedia table
    feat1_hist num_varray_256;
    feat1_ratio_rouge NUMBER;
    feat1_ratio_vert NUMBER;
    feat1_ratio_bleu NUMBER;
    feat1_moyenne_gradient_norme NUMBER;
    feat1_densite_contours NUMBER;
    feat1_est_couleur NUMBER;
    
    feat2_hist num_varray_256;
    feat2_ratio_rouge NUMBER;
    feat2_ratio_vert NUMBER;
    feat2_ratio_bleu NUMBER;
    feat2_moyenne_gradient_norme NUMBER;
    feat2_densite_contours NUMBER;
    feat2_est_couleur NUMBER;
    
    -- Computed values
    dist_hist NUMBER;
    diff_r NUMBER;
    diff_g NUMBER;
    diff_b NUMBER;
    diff_norm NUMBER;
    diff_contour NUMBER;
    diff_color NUMBER;
    score NUMBER;
BEGIN
    -- Fetch features for feat1
    SELECT hist, ratio_rouge, ratio_vert, ratio_bleu, moyenne_gradient_norme, densite_contours, est_couleur
    INTO feat1_hist, feat1_ratio_rouge, feat1_ratio_vert, feat1_ratio_bleu, feat1_moyenne_gradient_norme, feat1_densite_contours, feat1_est_couleur
    FROM multimedia
    WHERE nom = feat1_nom;
    
    -- Fetch features for feat2
    SELECT hist, ratio_rouge, ratio_vert, ratio_bleu, moyenne_gradient_norme, densite_contours, est_couleur
    INTO feat2_hist, feat2_ratio_rouge, feat2_ratio_vert, feat2_ratio_bleu, feat2_moyenne_gradient_norme, feat2_densite_contours, feat2_est_couleur
    FROM multimedia
    WHERE nom = feat2_nom;
    
    -- Compute histogram distance using Bhattacharyya
    dist_hist := distance_bhattacharyya(feat1_hist, feat2_hist);
    
    -- Compute absolute differences for RGB ratios
    diff_r := ABS(feat1_ratio_rouge - feat2_ratio_rouge);
    diff_g := ABS(feat1_ratio_vert - feat2_ratio_vert);
    diff_b := ABS(feat1_ratio_bleu - feat2_ratio_bleu);
    
    -- Compute differences for gradient norm and contour density
    diff_norm := ABS(feat1_moyenne_gradient_norme - feat2_moyenne_gradient_norme);
    diff_contour := ABS(feat1_densite_contours - feat2_densite_contours);
    
    -- Compute color penalty
    diff_color := CASE WHEN feat1_est_couleur != feat2_est_couleur THEN 1.0 ELSE 0.0 END;
    
    -- Compute weighted score
    score := weight_hist * dist_hist +
             weight_r * diff_r +
             weight_g * diff_g +
             weight_b * diff_b +
             weight_norm * diff_norm +
             weight_contour * diff_contour +
             weight_color * diff_color;
    
    RETURN score;
EXCEPTION
    WHEN NO_DATA_FOUND THEN
        RAISE_APPLICATION_ERROR(-20001, 'One or both images not found in multimedia table');
END;
/

-- Type for returning ranked results (nom and score)
CREATE OR REPLACE TYPE image_score_rec AS OBJECT (
    nom VARCHAR2(100),
    score NUMBER
);
/

CREATE OR REPLACE TYPE image_score_table AS TABLE OF image_score_rec;
/

-- Function to rank similar images for a given image
CREATE OR REPLACE FUNCTION rank_similar_images(
    target_nom IN VARCHAR2,
    weight_hist IN NUMBER DEFAULT 1.0,
    weight_r IN NUMBER DEFAULT 1.0,
    weight_g IN NUMBER DEFAULT 1.0,
    weight_b IN NUMBER DEFAULT 1.0,
    weight_norm IN NUMBER DEFAULT 1.0,
    weight_contour IN NUMBER DEFAULT 1.0,
    weight_color IN NUMBER DEFAULT 1.0,
    top_n IN NUMBER DEFAULT NULL,  -- Limit to top N results (NULL for all)
    min_score IN NUMBER DEFAULT NULL  -- Only include scores below this threshold (NULL for no filter)
) RETURN image_score_table PIPELINED IS
    CURSOR img_cursor IS
        SELECT nom FROM multimedia WHERE nom != target_nom;
    scores image_score_table := image_score_table();
    current_score NUMBER;
BEGIN
    FOR img_rec IN img_cursor LOOP
        current_score := evaluate_score_plsql(target_nom, img_rec.nom, weight_hist, weight_r, weight_g, weight_b, weight_norm, weight_contour, weight_color);
        
        -- Apply filters
        IF (min_score IS NULL OR current_score < min_score) THEN
            scores.EXTEND;
            scores(scores.LAST) := image_score_rec(img_rec.nom, current_score);
        END IF;
    END LOOP;
    
    -- Sort by score ascending (lower = more similar)
    -- Note: PL/SQL collections don't have built-in sort, so we use a simple bubble sort here for small datasets
    FOR i IN 1..scores.COUNT LOOP
        FOR j IN i+1..scores.COUNT LOOP
            IF scores(j).score < scores(i).score THEN
                -- Swap
                DECLARE
                    temp image_score_rec := scores(i);
                BEGIN
                    scores(i) := scores(j);
                    scores(j) := temp;
                END;
            END IF;
        END LOOP;
    END LOOP;
    
    -- Pipe top N results
    FOR i IN 1..LEAST(NVL(top_n, scores.COUNT), scores.COUNT) LOOP
        PIPE ROW(scores(i));
    END LOOP;
    
    RETURN;
END;
/

-- Optional: Custom sorting function if you need more control (e.g., sort by multiple criteria)
-- This sorts a collection by score, but you can extend it
CREATE OR REPLACE PROCEDURE sort_image_scores(scores IN OUT image_score_table) IS
BEGIN
    FOR i IN 1..scores.COUNT LOOP
        FOR j IN i+1..scores.COUNT LOOP
            IF scores(j).score < scores(i).score THEN
                DECLARE
                    temp image_score_rec := scores(i);
                BEGIN
                    scores(i) := scores(j);
                    scores(j) := temp;
                END;
            END IF;
        END LOOP;
    END LOOP;
END;
/



--------------------------------------------------------------------- Requête score avec nouvelles colonnes  


--7. recherche d’images similaires (Top-10) pour une image donnée
SET SERVEROUTPUT ON;

DECLARE
  TYPE t_score IS RECORD (
    nom   VARCHAR2(200),
    score NUMBER
  );
  TYPE t_tab IS TABLE OF t_score INDEX BY PLS_INTEGER;

  scores      t_tab;
  refsig      ORDSYS.ORDIMAGESIGNATURE;
  i           PLS_INTEGER := 0;
  tmp         t_score;
  v_image_nom VARCHAR2(200) := '382.jpg';  -- <<< une seule fois ici
BEGIN
  -- Signature de référence
  SELECT signature INTO refsig
  FROM multimedia
  WHERE FILENAME = v_image_nom;

  -- Calculer tous les scores
  FOR rec IN (SELECT FILENAME , signature FROM multimedia WHERE FILENAME <> v_image_nom) LOOP
    i := i + 1;
    scores(i).nom   := rec.FILENAME;
    scores(i).score := ORDSYS.ORDIMAGESIGNATURE.evaluateScore(
                         refsig,
                         rec.signature,
                         'color=0.1,texture=0.4,shape=0.4,location=0.1'
                       );
  END LOOP;

  -- Tri à bulles (simple mais OK pour quelques centaines d’images)
  FOR a IN 1..i LOOP
    FOR b IN a+1..i LOOP
      IF scores(b).score < scores(a).score THEN
        tmp := scores(a);
        scores(a) := scores(b);
        scores(b) := tmp;
      END IF;
    END LOOP;
  END LOOP;

  -- Afficher le Top-10
  DBMS_OUTPUT.PUT_LINE('--- Top 10 similaires à ' || v_image_nom || ' ---');
  FOR k IN 1..LEAST(10,i) LOOP
    DBMS_OUTPUT.PUT_LINE(scores(k).nom || ' => score = ' || scores(k).score);
  END LOOP;
END;
/





------- SCRIPT Table features ... avant jointure 


-- Adapted script for the Features table
-- Assumptions:
-- - HISTOGRAM256 is the same as num_varray_256 (VARRAY(256) OF NUMBER)
-- - Columns like MOYENNE_GRADIENT_NORME, DENSITE_CONTOURS, etc., are stored as VARCHAR2 but contain numeric values; we'll cast them to NUMBER in queries
-- - The table is named "Features" (case-sensitive in Oracle)
-- - "nom" is replaced with "filename" to match the schema
-- - No "signature" column in Features, so the final block (using ORDSYS.ORDIMAGESIGNATURE) is removed or adapted to use the custom scoring function instead
-- - distance_bhattacharyya function is assumed to exist (not defined here)

CREATE OR REPLACE FUNCTION distance_bhattacharyya(
    hist1 IN HISTOGRAM256,
    hist2 IN HISTOGRAM256
) RETURN NUMBER IS
    sum_sqrt NUMBER := 0;
    epsilon  CONSTANT NUMBER := 1e-10;
BEGIN
    IF hist1 IS NULL OR hist2 IS NULL OR hist1.COUNT != 256 OR hist2.COUNT != 256 THEN
        RAISE_APPLICATION_ERROR(-20003, 'Histograms invalides (NULL ou ≠ 256 éléments)');
    END IF;

    FOR i IN 1..256 LOOP
        sum_sqrt := sum_sqrt + SQRT(hist1(i) * hist2(i));
    END LOOP;

    RETURN -LN(sum_sqrt + epsilon);
END;
/


CREATE OR REPLACE FUNCTION evaluate_score_plsql(
    feat1_filename IN VARCHAR2,
    feat2_filename IN VARCHAR2,
    weight_hist    IN NUMBER,
    weight_r       IN NUMBER,
    weight_g       IN NUMBER,
    weight_b       IN NUMBER,
    weight_norm    IN NUMBER,
    weight_contour IN NUMBER,
    weight_color   IN NUMBER
) RETURN NUMBER IS
    -- Features de feat1
    feat1_hist HISTOGRAM256;
    feat1_ratio_rouge NUMBER;
    feat1_ratio_vert  NUMBER;
    feat1_ratio_bleu  NUMBER;
    feat1_moyenne_gradient_norme NUMBER;
    feat1_densite_contours       NUMBER;
    feat1_est_couleur            NUMBER;

    -- Features de feat2
    feat2_hist HISTOGRAM256;
    feat2_ratio_rouge NUMBER;
    feat2_ratio_vert  NUMBER;
    feat2_ratio_bleu  NUMBER;
    feat2_moyenne_gradient_norme NUMBER;
    feat2_densite_contours       NUMBER;
    feat2_est_couleur            NUMBER;

    -- Variables calcul
    dist_hist   NUMBER;
    diff_r      NUMBER;
    diff_g      NUMBER;
    diff_b      NUMBER;
    diff_norm   NUMBER;
    diff_contour NUMBER;
    diff_color  NUMBER;
    score       NUMBER;
BEGIN
    -- Récupération image 1
    SELECT histogramme_varray,
           ratio_rouge,
           ratio_vert,
           ratio_bleu,
           moyenne_gradient_norme,
           densite_contours,
           est_couleur
    INTO feat1_hist, feat1_ratio_rouge, feat1_ratio_vert, feat1_ratio_bleu,
         feat1_moyenne_gradient_norme, feat1_densite_contours, feat1_est_couleur
    FROM Features
    WHERE filename = feat1_filename;

    -- Récupération image 2
    SELECT histogramme_varray,
           ratio_rouge,
           ratio_vert,
           ratio_bleu,
           moyenne_gradient_norme,
           densite_contours,
           est_couleur
    INTO feat2_hist, feat2_ratio_rouge, feat2_ratio_vert, feat2_ratio_bleu,
         feat2_moyenne_gradient_norme, feat2_densite_contours, feat2_est_couleur
    FROM Features
    WHERE filename = feat2_filename;

    -- Distance Bhattacharyya
    dist_hist := distance_bhattacharyya(feat1_hist, feat2_hist);

    -- Différences
    diff_r      := ABS(feat1_ratio_rouge - feat2_ratio_rouge);
    diff_g      := ABS(feat1_ratio_vert  - feat2_ratio_vert);
    diff_b      := ABS(feat1_ratio_bleu  - feat2_ratio_bleu);
    diff_norm   := ABS(feat1_moyenne_gradient_norme - feat2_moyenne_gradient_norme);
    diff_contour:= ABS(feat1_densite_contours - feat2_densite_contours);
    diff_color  := CASE WHEN feat1_est_couleur != feat2_est_couleur THEN 1 ELSE 0 END;

    -- Score final pondéré
    score := weight_hist * dist_hist
           + weight_r * diff_r
           + weight_g * diff_g
           + weight_b * diff_b
           + weight_norm * diff_norm
           + weight_contour * diff_contour
           + weight_color * diff_color;

    RETURN score;

EXCEPTION
    WHEN NO_DATA_FOUND THEN
        RAISE_APPLICATION_ERROR(-20001, 'Une des images est absente de Features');
    WHEN OTHERS THEN
        RAISE_APPLICATION_ERROR(-20002, 'Erreur dans evaluate_score_plsql: ' || SQLERRM);
END;
/


CREATE OR REPLACE FUNCTION rank_similar_images(
    target_filename IN VARCHAR2,
    weight_hist    IN NUMBER DEFAULT 1,
    weight_r       IN NUMBER DEFAULT 1,
    weight_g       IN NUMBER DEFAULT 1,
    weight_b       IN NUMBER DEFAULT 1,
    weight_norm    IN NUMBER DEFAULT 1,
    weight_contour IN NUMBER DEFAULT 1,
    weight_color   IN NUMBER DEFAULT 1,
    top_n          IN NUMBER DEFAULT NULL
) RETURN image_score_table PIPELINED IS
    CURSOR img_cursor IS
        SELECT filename FROM Features WHERE filename != target_filename;
    current_score NUMBER;
BEGIN
    FOR img_rec IN img_cursor LOOP
        current_score := evaluate_score_plsql(
            target_filename, img_rec.filename,
            weight_hist, weight_r, weight_g, weight_b,
            weight_norm, weight_contour, weight_color
        );

        PIPE ROW(image_score_rec(img_rec.filename, current_score));
    END LOOP;
    RETURN;
END;
/


SET SERVEROUTPUT ON;

CREATE OR REPLACE PROCEDURE afficher_top_similaires(
    p_target_filename IN VARCHAR2,
    p_top_n           IN NUMBER DEFAULT 10
) IS
BEGIN
    DBMS_OUTPUT.PUT_LINE('--- Top ' || p_top_n || ' images similaires à ' || p_target_filename || ' ---');

    FOR rec IN (
        SELECT *
        FROM TABLE(rank_similar_images(
            p_target_filename,
            1,  -- poids histogramme
            1,  -- poids ratio R
            1,  -- poids ratio G
            1,  -- poids ratio B
            1,  -- poids gradient
            1,  -- poids contour
            1,  -- poids couleur
            p_top_n
        ))
    ) LOOP
        DBMS_OUTPUT.PUT_LINE(RPAD(rec.filename, 30) || ' => score = ' || TO_CHAR(rec.score, 'FM9990.0000'));
    END LOOP;
END;
/


BEGIN
    afficher_top_similaires('1.jpg', 10);
END;
/






------------------------ travail sur la partie temps d'exécution 


-- Modified version of evaluate_score_plsql with detailed prints for debugging (same as before)
CREATE OR REPLACE FUNCTION evaluate_score_plsql(
    feat1_nom IN VARCHAR2,
    feat2_nom IN VARCHAR2,
    weight_hist IN NUMBER,
    weight_r IN NUMBER,
    weight_g IN NUMBER,
    weight_b IN NUMBER,
    weight_norm IN NUMBER,
    weight_contour IN NUMBER,
    weight_color IN NUMBER
) RETURN NUMBER IS
    -- Variables for features from multimedia table
    feat1_hist HISTOGRAM256;
    feat1_ratio_rouge NUMBER;
    feat1_ratio_vert NUMBER;
    feat1_ratio_bleu NUMBER;
    feat1_moyenne_gradient_norme NUMBER;
    feat1_densite_contours NUMBER;
    feat1_est_couleur NUMBER;
    
    feat2_hist HISTOGRAM256;
    feat2_ratio_rouge NUMBER;
    feat2_ratio_vert NUMBER;
    feat2_ratio_bleu NUMBER;
    feat2_moyenne_gradient_norme NUMBER;
    feat2_densite_contours NUMBER;
    feat2_est_couleur NUMBER;
    
    -- Computed values
    dist_hist NUMBER;
    diff_r NUMBER;
    diff_g NUMBER;
    diff_b NUMBER;
    diff_norm NUMBER;
    diff_contour NUMBER;
    diff_color NUMBER;
    score NUMBER;
BEGIN
    -- Fetch features for feat1
    SELECT histogram, ratio_rouge, ratio_vert, ratio_bleu, moyenne_gradient_norme, densite_contours, est_couleur
    INTO feat1_hist, feat1_ratio_rouge, feat1_ratio_vert, feat1_ratio_bleu, feat1_moyenne_gradient_norme, feat1_densite_contours, feat1_est_couleur
    FROM multimedia
    WHERE filename = feat1_nom;
    
    -- Fetch features for feat2
    SELECT histogram, ratio_rouge, ratio_vert, ratio_bleu, moyenne_gradient_norme, densite_contours, est_couleur
    INTO feat2_hist, feat2_ratio_rouge, feat2_ratio_vert, feat2_ratio_bleu, feat2_moyenne_gradient_norme, feat2_densite_contours, feat2_est_couleur
    FROM multimedia
    WHERE filename = feat2_nom;
    
    -- Compute histogram distance using Bhattacharyya
    dist_hist := distance_bhattacharyya(feat1_hist, feat2_hist);
    DBMS_OUTPUT.PUT_LINE('Distance histogramme pour ' || feat1_nom || ' vs ' || feat2_nom || ': ' || dist_hist);
    
    -- Compute absolute differences for RGB ratios
    diff_r := ABS(feat1_ratio_rouge - feat2_ratio_rouge);
    DBMS_OUTPUT.PUT_LINE('Différence ratio rouge: ' || diff_r);
    diff_g := ABS(feat1_ratio_vert - feat2_ratio_vert);
    DBMS_OUTPUT.PUT_LINE('Différence ratio vert: ' || diff_g);
    diff_b := ABS(feat1_ratio_bleu - feat2_ratio_bleu);
    DBMS_OUTPUT.PUT_LINE('Différence ratio bleu: ' || diff_b);
    
    -- Compute differences for gradient norm and contour density
    diff_norm := ABS(feat1_moyenne_gradient_norme - feat2_moyenne_gradient_norme);
    DBMS_OUTPUT.PUT_LINE('Différence norme gradient: ' || diff_norm);
    diff_contour := ABS(feat1_densite_contours - feat2_densite_contours);
    DBMS_OUTPUT.PUT_LINE('Différence densité contours: ' || diff_contour);
    
    -- Compute color penalty
    diff_color := CASE WHEN feat1_est_couleur != feat2_est_couleur THEN 1.0 ELSE 0.0 END;
    DBMS_OUTPUT.PUT_LINE('Pénalité couleur: ' || diff_color);
    
    -- Compute weighted score
    score := weight_hist * dist_hist +
             weight_r * diff_r +
             weight_g * diff_g +
             weight_b * diff_b +
             weight_norm * diff_norm +
             weight_contour * diff_contour +
             weight_color * diff_color;
    DBMS_OUTPUT.PUT_LINE('Score total pour ' || feat1_nom || ' vs ' || feat2_nom || ': ' || score);
    
    RETURN score;
EXCEPTION
    WHEN NO_DATA_FOUND THEN
        RAISE_APPLICATION_ERROR(-20001, 'One or both images not found in multimedia table');
END;
/

-- Modified afficher_top_similaires to display ALL distances from lowest to highest (not just top 10)
CREATE OR REPLACE PROCEDURE afficher_top_similaires(
    p_target_filename IN VARCHAR2,
    p_top_n IN NUMBER DEFAULT 10  -- Set to NULL to display all
) IS
    TYPE t_score IS RECORD (
        filename VARCHAR2(100),
        score    NUMBER
    );
    TYPE t_tab IS TABLE OF t_score INDEX BY PLS_INTEGER;
    scores t_tab;
    i      PLS_INTEGER := 0;
    tmp    t_score;
BEGIN
    -- Calculer tous les scores (avec prints dans evaluate_score_plsql)
    FOR rec IN (SELECT filename FROM multimedia WHERE filename <> p_target_filename) LOOP
        i := i + 1;
        scores(i).filename := rec.filename;
        scores(i).score := evaluate_score_plsql(
            p_target_filename,
            rec.filename,
            0.5,  -- poids histogramme
            0.200,  -- poids ratio R
            0.2,  -- poids ratio G
            0.3,  -- poids ratio B
            0.1,  -- poids gradient
            0.1,  -- poids contour
            0.05   -- poids couleur
        );
    END LOOP;

    -- Tri à bulles (simple mais OK pour quelques centaines d’images) - ascending (lowest first)
    FOR a IN 1..i LOOP
        FOR b IN a+1..i LOOP
            IF scores(b).score < scores(a).score THEN
                tmp := scores(a);
                scores(a) := scores(b);
                scores(b) := tmp;
            END IF;
        END LOOP;
    END LOOP;

    -- Afficher toutes les distances des plus basses aux plus hautes
    DBMS_OUTPUT.PUT_LINE('--- Toutes les distances pour ' || p_target_filename || ' (du plus bas au plus haut) ---');
    FOR k IN 1..p_top_n LOOP
        DBMS_OUTPUT.PUT_LINE(scores(k).filename || ' => score = ' || scores(k).score);
    END LOOP;
END;
/

BEGIN
    afficher_top_similaires('65.jpg', 10);  -- NULL for all results
END;
/









---------------------------------------------------------------- Temps exécution--------------------------------------------------------------------------------

SET SERVEROUTPUT ON
DECLARE
  -- === Chrono en centi-secondes (1/100 s) ===
  t_start  PLS_INTEGER;
  t_after_calc PLS_INTEGER;
  t_after_sort PLS_INTEGER;

  -- === Cible & Top-N ===
  v_target VARCHAR2(260) := '5.jpg';
  v_topn   PLS_INTEGER   := 10;

  -- === Structures ===
  TYPE t_score IS RECORD (filename VARCHAR2(300), score NUMBER);
  TYPE t_tab   IS TABLE  OF t_score INDEX BY PLS_INTEGER;

  scores t_tab;
  i      PLS_INTEGER := 0;
  tmp    t_score;

BEGIN
  DBMS_OUTPUT.PUT_LINE('=== [FEATURES] Top-'||v_topn||' similaires à '||v_target||' ===');

  t_start := DBMS_UTILITY.get_time;

  -- 1) Calcul des scores (features)
  FOR rec IN (
    SELECT filename
    FROM multimedia
  ) LOOP
    i := i + 1;
    scores(i).filename := rec.filename;
    scores(i).score := evaluate_score_plsql(
      v_target, rec.filename,
      1, 1, 1, 1, 1, 1, 1  -- poids = 1 pour comparabilité
    );
  END LOOP;

  t_after_calc := DBMS_UTILITY.get_time;

  -- 2) Tri bubble (OK pour petit jeu)
  FOR a IN 1..i LOOP
    FOR b IN a+1..i LOOP
      IF scores(b).score < scores(a).score THEN
        tmp := scores(a); scores(a) := scores(b); scores(b) := tmp;
      END IF;
    END LOOP;
  END LOOP;

  t_after_sort := DBMS_UTILITY.get_time;

  -- 3) Affichage Top-N
  FOR k IN 1..LEAST(v_topn, i) LOOP
    DBMS_OUTPUT.PUT_LINE(
      LPAD(k,2)||'. '||scores(k).filename||' => score='||TO_CHAR(scores(k).score,'0.000000')
    );
  END LOOP;

  -- 4) Stats temps
  DBMS_OUTPUT.PUT_LINE('---');
  DBMS_OUTPUT.PUT_LINE('Paires évaluées     : '||i);
  DBMS_OUTPUT.PUT_LINE('Temps scoring (ms)  : '|| (t_after_calc - t_start)*10);
  DBMS_OUTPUT.PUT_LINE('Temps tri (ms)      : '|| (t_after_sort - t_after_calc)*10);
  DBMS_OUTPUT.PUT_LINE('Temps total (ms)    : '|| (t_after_sort - t_start)*10);
  IF i > 0 THEN
    DBMS_OUTPUT.PUT_LINE('Moyenne/pair (ms)   : '|| ROUND(((t_after_calc - t_start)*10)/i,3));
  END IF;
END;
/



DECLARE
  -- === Chrono ===
  t_start       PLS_INTEGER;
  t_after_calc  PLS_INTEGER;
  t_after_sort  PLS_INTEGER;

  -- === Types ===
  TYPE t_score IS RECORD (
    nom   VARCHAR2(200),
    score NUMBER
  );
  TYPE t_tab IS TABLE OF t_score INDEX BY PLS_INTEGER;

  -- === Vars ===
  scores      t_tab;
  refsig      ORDSYS.ORDIMAGESIGNATURE;
  i           PLS_INTEGER := 0;
  tmp         t_score;
  v_image_nom VARCHAR2(200) := '5.jpg';  -- <<< change si besoin

  v_params    VARCHAR2(200) := 'color=0.1,texture=0.4,shape=0.4,location=0.1';
  v_topn      PLS_INTEGER := 10;
BEGIN
  DBMS_OUTPUT.PUT_LINE('=== [ORACLE SIGNATURE] Top-'||v_topn||' similaires à '||v_image_nom||' ===');

  -- Signature de référence
  SELECT signature INTO refsig
  FROM multimedia
  WHERE filename = v_image_nom;

  t_start := DBMS_UTILITY.get_time;

  -- 1) Calculer tous les scores (signature ORACLE)
  FOR rec IN (
    SELECT filename, signature
    FROM multimedia
  ) LOOP
    i := i + 1;
    scores(i).nom   := rec.filename;
    scores(i).score := ORDSYS.ORDIMAGESIGNATURE.evaluateScore(
                         refsig,
                         rec.signature,
                         v_params
                       );
  END LOOP;

  t_after_calc := DBMS_UTILITY.get_time;

  -- 2) Tri (bubble)
  FOR a IN 1..i LOOP
    FOR b IN a+1..i LOOP
      IF scores(b).score < scores(a).score THEN
        tmp := scores(a); scores(a) := scores(b); scores(b) := tmp;
      END IF;
    END LOOP;
  END LOOP;

  t_after_sort := DBMS_UTILITY.get_time;

  -- 3) Afficher le Top-10
  FOR k IN 1..LEAST(v_topn,i) LOOP
    DBMS_OUTPUT.PUT_LINE(
      LPAD(k,2)||'. '||scores(k).nom||' => score='||TO_CHAR(scores(k).score,'0.000000')
    );
  END LOOP;

  -- 4) Stats temps
  DBMS_OUTPUT.PUT_LINE('---');
  DBMS_OUTPUT.PUT_LINE('Paires évaluées     : '||i);
  DBMS_OUTPUT.PUT_LINE('Temps scoring (ms)  : '|| (t_after_calc - t_start)*10);
  DBMS_OUTPUT.PUT_LINE('Temps tri (ms)      : '|| (t_after_sort - t_after_calc)*10);
  DBMS_OUTPUT.PUT_LINE('Temps total (ms)    : '|| (t_after_sort - t_start)*10);
  IF i > 0 THEN
    DBMS_OUTPUT.PUT_LINE('Moyenne/pair (ms)   : '|| ROUND(((t_after_calc - t_start)*10)/i,3));
  END IF;
END;
/

---------------------------------------------------