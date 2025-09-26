import pandas as pd

# Chemin vers ton fichier CSV (ajuste selon ton système)
csv_file = 'features_all.csv'  # Remplace par le chemin réel, ex. 'C:\\temp\\features_all.csv'

# Lis le CSV (assume que la première ligne est l'en-tête)
df = pd.read_csv(csv_file, sep=',')

# Fonction pour parser la colonne 'histogram' en liste de nombres
def parse_histogram(hist_str):
    if pd.isna(hist_str) or hist_str.strip() == '':
        return []  # Si vide, retourne une liste vide
    try:
        # Retire les guillemets externes, puis les parenthèses, puis split sur virgule
        cleaned_str = hist_str.strip().strip('"').lstrip('(').rstrip(')')
        # Split par virgule et convertit en float
        values = [str(float(x.strip())) for x in cleaned_str.split(',') if x.strip()]
        return values
    except ValueError:
        print(f"Erreur de parsing pour histogram: {hist_str}. Ignoré.")
        return []

# Génère les instructions SQL
sql_statements = []
for index, row in df.iterrows():
    filename = row['filename']
    try:
        # Plus besoin de row_id, on utilise directement filename
        pass  # On utilise filename dans le WHERE
    except (ValueError, IndexError):
        print(f"Erreur : filename '{filename}' n'est pas valide. Ignoré.")
        continue
    
    hist_values = parse_histogram(row['histogram'])
    
    if hist_values:
        # Crée la liste pour VARRAY, ex. : histogram_varray(1,2,3,4)
        varray_values = ', '.join(hist_values)
        sql = f"UPDATE Features SET histogram_new = histogram_varray({varray_values}) WHERE filename = '{filename}';"
    else:
        # Si pas de valeurs, insère un VARRAY vide
        sql = f"UPDATE Features SET histogram_new = histogram_varray() WHERE filename = '{filename}';"
    
    sql_statements.append(sql)

# Affiche les instructions SQL (copie-les manuellement)
print("Instructions SQL générées (copie et exécute-les une par une dans Oracle) :")
for stmt in sql_statements:
    print(stmt)

# Optionnel : Écris dans un fichier pour faciliter la copie
with open('generated_updates.sql', 'w') as f:
    f.write('\n'.join(sql_statements))
print("\nLes instructions ont aussi été écrites dans 'generated_updates.sql'.")