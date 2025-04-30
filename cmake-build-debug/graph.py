import pandas as pd
import matplotlib.pyplot as plt

# Cargar resultados desde archivo CSV
# Formato esperado:
# tiempo, pe_id, mensajes, datos_bytes, tipo_mensaje
df = pd.read_csv("resultados_simulacion.csv")

# Ancho de banda a lo largo del tiempo
bw = df.groupby("tiempo")["datos_bytes"].sum()
plt.figure(figsize=(10, 5))
plt.plot(bw.index, bw.values, marker="o")
plt.title("Ancho de Banda del Interconnect")
plt.xlabel("Tiempo (unidades simuladas)")
plt.ylabel("Bytes transmitidos")
plt.grid(True)
plt.savefig("ancho_de_banda.png")
plt.show()

# Tráfico total por PE
trafico_pe = df.groupby("pe_id")["datos_bytes"].sum()
plt.figure(figsize=(10, 5))
trafico_pe.plot(kind="bar", color="orange")
plt.title("Tráfico Total por PE")
plt.xlabel("PE ID")
plt.ylabel("Bytes transmitidos")
plt.grid(axis='y')
plt.savefig("trafico_por_pe.png")
plt.show()

# Conteo de mensajes por tipo
mensajes_tipo = df["tipo_mensaje"].value_counts()
plt.figure(figsize=(8, 5))
mensajes_tipo.plot(kind="pie", autopct='%1.1f%%', startangle=140)
plt.title("Distribución de Tipos de Mensaje")
plt.ylabel("")
plt.savefig("tipos_de_mensaje.png")
plt.show()