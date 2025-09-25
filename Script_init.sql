--1. Création du type pour l’histogramme
CREATE OR REPLACE TYPE HISTOGRAM256 AS VARRAY(256) OF NUMBER;
/

---2. Création de la table MULTIMEDIA

CREATE TABLE MULTIMEDIA (
  ID                    NUMBER PRIMARY KEY,
  FILENAME              VARCHAR2(200) UNIQUE NOT NULL,
  IMAGE                 ORDSYS.ORDIMAGE,
  SIGNATURE             ORDSYS.ORDIMAGESIGNATURE,
  -- Caractéristiques du CSV
  WIDTH                 NUMBER,
  HEIGHT                NUMBER,
  MOYENNE_GRADIENT_NORME NUMBER,
  DENSITE_CONTOURS      NUMBER,
  RATIO_ROUGE           NUMBER,
  RATIO_VERT            NUMBER,
  RATIO_BLEU            NUMBER,
  EST_COULEUR           NUMBER,
  HISTOGRAM             HISTOGRAM256
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
    INSERT INTO MULTIMEDIA (FILENAME, IMAGE, SIGNATURE)
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
      WHERE FILENAME = TO_CHAR(i) || '.jpg'
      FOR UPDATE;

      img.importFrom(ctx, 'file', 'IMG', TO_CHAR(i) || '.jpg');

      UPDATE MULTIMEDIA
      SET IMAGE = img
      WHERE FILENAME = TO_CHAR(i) || '.jpg';

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
  CURSOR c IS SELECT ROWID rid, IMAGE, SIGNATURE sig, FILENAME FROM MULTIMEDIA FOR UPDATE;
BEGIN
  FOR r IN c LOOP
    BEGIN
      -- Initialiser la signature si NULL
      IF r.sig IS NULL THEN
        r.sig := ORDSYS.ORDIMAGESIGNATURE.INIT();
      END IF;
      r.sig.generateSignature(r.IMAGE);
      UPDATE MULTIMEDIA
      SET SIGNATURE = r.sig
      WHERE ROWID = r.rid;
    EXCEPTION
      WHEN OTHERS THEN
        DBMS_OUTPUT.PUT_LINE('Erreur signature pour '||r.FILENAME||' : '||SQLERRM);
    END;
  END LOOP;
  COMMIT;
END;
/



