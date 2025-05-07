import os

# Ruta a la carpeta que contiene los archivos
outputDir = "../output"

# Nombres de los archivos a eliminar
fileNames = ["intconnect.txt"] + [f"pe{i}.txt" for i in range(8)]

for fileName in fileNames:
    filePath = os.path.join(outputDir, fileName)
    try:
        os.remove(filePath)
        print(f"Eliminado: {filePath}")
    except FileNotFoundError:
        print(f"No existe: {filePath}")
    except Exception as e:
        print(f"Error al eliminar {filePath}: {e}")