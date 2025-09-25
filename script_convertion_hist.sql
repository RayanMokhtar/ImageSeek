-- 0) Attendre un verrou DDL si besoin + forcer le point décimal
ALTER SESSION SET ddl_lock_timeout = 60;
ALTER SESSION SET nls_numeric_characters='.,';

--------------------------------------------------------------------------------
-- 1) Créer la colonne HIST_TEST si elle n'existe pas
--------------------------------------------------------------------------------
DECLARE
  v_exists NUMBER;
BEGIN
  SELECT COUNT(*) INTO v_exists
  FROM   user_tab_cols
  WHERE  table_name  = 'FEATURES'
  AND    column_name = 'HIST_TEST';

  IF v_exists = 0 THEN
    EXECUTE IMMEDIATE 'ALTER TABLE FEATURES ADD (HIST_TEST HISTOGRAM256)';
  END IF;
END;
/
--------------------------------------------------------------------------------
-- 2) Fonction rapide sans REGEXP (INSTR/SUBSTR), sans PRAGMA
--------------------------------------------------------------------------------
CREATE OR REPLACE FUNCTION to_hist256_fast(p_raw IN VARCHAR2)
  RETURN HISTOGRAM256
IS
  v_arr   HISTOGRAM256 := HISTOGRAM256();
  v_str   VARCHAR2(32767);
  v_len   PLS_INTEGER;
  i       PLS_INTEGER;  -- position courante (1-based)
  j       PLS_INTEGER;  -- position de la prochaine virgule
  tok     VARCHAR2(4000);
  numval  NUMBER;
BEGIN
  IF p_raw IS NULL THEN
    RETURN NULL;
  END IF;

  v_str := TRIM(p_raw);
  IF v_str IS NULL THEN
    RETURN NULL;
  END IF;

  -- enlever crochets JSON éventuels
  IF SUBSTR(v_str,1,1) = '[' THEN
    v_str := SUBSTR(v_str,2);
  END IF;
  IF SUBSTR(v_str,-1,1) = ']' THEN
    v_str := SUBSTR(v_str,1,LENGTH(v_str)-1);
  END IF;

  v_len := LENGTH(v_str);
  IF v_len IS NULL OR v_len = 0 THEN
    RETURN NULL;
  END IF;

  i := 1;
  WHILE i <= v_len LOOP
    j := INSTR(v_str, ',', i);
    IF j = 0 THEN
      tok := SUBSTR(v_str, i);          -- dernier token
      i := v_len + 1;
    ELSE
      tok := SUBSTR(v_str, i, j - i);   -- token courant
      i := j + 1;
    END IF;

    tok := REPLACE(REPLACE(TRIM(tok), CHR(9), ''), '"', '');
    -- nls_numeric_characters='.,' déjà réglé en session
    IF tok IS NOT NULL THEN
      BEGIN
        numval := TO_NUMBER(tok);
        v_arr.EXTEND;
        v_arr(v_arr.COUNT) := numval;
        IF v_arr.COUNT >= 256 THEN
          EXIT;
        END IF;
      EXCEPTION WHEN OTHERS THEN
        NULL; -- token non numérique: ignorer
      END;
    END IF;
  END LOOP;

  IF v_arr.COUNT = 0 THEN
    RETURN NULL;
  ELSE
    RETURN v_arr;
  END IF;
END to_hist256_fast;
/
SHOW ERRORS;

--------------------------------------------------------------------------------
-- 3) Mise à jour par paquets pour éviter gros UNDO/REDO (ajuste v_batch si besoin)
--------------------------------------------------------------------------------
DECLARE
  v_rows  PLS_INTEGER;
  v_batch CONSTANT PLS_INTEGER := 20000; -- 10k–50k selon ressources
BEGIN
  LOOP
    UPDATE /*+ PARALLEL(f,8) */ FEATURES f
       SET HIST_TEST = to_hist256_fast(f.HISTOGRAM)
     WHERE f.HISTOGRAM IS NOT NULL
       AND f.HIST_TEST IS NULL     -- ne traite pas ce qui est déjà rempli
       AND ROWNUM <= v_batch;

    v_rows := SQL%ROWCOUNT;
    COMMIT;
    EXIT WHEN v_rows = 0;
  END LOOP;
END;
/



-- Enrichissement de MULTIMEDIA depuis la table FEATURES
SET SERVEROUTPUT ON;
BEGIN
  MERGE INTO MULTIMEDIA m
  USING FEATURES f
    ON (m.FILENAME = f.FILENAME)
  WHEN MATCHED THEN
    UPDATE SET
      m.WIDTH = f.WIDTH,
      m.HEIGHT = f.HEIGHT,
      m.MOYENNE_GRADIENT_NORME = f.MOYENNE_GRADIENT_NORME,
      m.DENSITE_CONTOURS = f.DENSITE_CONTOURS,
      m.RATIO_ROUGE = f.RATIO_ROUGE,
      m.RATIO_VERT = f.RATIO_VERT,
      m.RATIO_BLEU = f.RATIO_BLEU,
      m.EST_COULEUR = f.EST_COULEUR,
  m.HISTOGRAM = f.HIST_TEST;
  DBMS_OUTPUT.PUT_LINE('MULTIMEDIA enrichi depuis FEATURES.');
END;
/
