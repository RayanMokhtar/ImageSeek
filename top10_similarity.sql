-- top10_similarity.sql
-- Affiche le Top-10 des images les plus similaires à une image donnée

SET SERVEROUTPUT ON SIZE 1000000;

DECLARE
  -- Nom du fichier de référence (changez-le selon vos besoins)
  v_image_nom VARCHAR2(200) := '382.jpg';
  refsig      ORDSYS.ORDIMAGESIGNATURE;
BEGIN
  -- Récupère la signature de l'image de référence
  SELECT SIGNATURE
    INTO refsig
    FROM MULTIMEDIA
   WHERE FILENAME = v_image_nom;

  DBMS_OUTPUT.PUT_LINE('--- Top 10 similaires à ' || v_image_nom || ' ---');

  -- Parcours du Top-10 des images les plus proches (score ascendant)
  FOR rec IN (
    SELECT * FROM (
      SELECT filename,
             ORDSYS.ORDIMAGESIGNATURE.evaluateScore(
               refsig,
               signature,
               'color=0.1,texture=0.4,shape=0.4,location=0.1'
             ) AS score
        FROM MULTIMEDIA
       WHERE filename <> v_image_nom
         AND signature IS NOT NULL
       ORDER BY score
    )
    WHERE ROWNUM <= 10
  ) LOOP
    DBMS_OUTPUT.PUT_LINE(rec.filename || ' => score = ' || rec.score);
  END LOOP;
END;
/
