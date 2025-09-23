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

---2. Création de la table MULTIMEDIA

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
