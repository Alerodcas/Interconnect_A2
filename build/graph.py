# script para generar gráficas del interconnect y PEs
import os
import pandas as pd
import matplotlib.pyplot as plt

# ruta de la carpeta con los archivos
folderPath = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "output"))

# función para cargar logs
def loadLogs(filePath):
    with open(filePath, 'r') as f:
        lines = f.readlines()[1:]  # saltar encabezado
    data = []
    for line in lines:
        parts = line.strip().split()
        if len(parts) == 5:
            instr, recvSend, size, srcDst, cycle = parts
            data.append({
                "instr": instr,
                "recvSend": int(recvSend),
                "size": int(size),
                "srcDst": srcDst,
                "cycle": int(cycle)
            })
    return pd.DataFrame(data)

# cargar interconnect y PEs
interconnect = loadLogs(os.path.join(folderPath, "intconnect.txt"))
pes = [loadLogs(os.path.join(folderPath, f"pe{i}.txt")) for i in range(8)]

# --- Gráfica 1: Ancho de banda por ciclo en interconnect ---
bw = interconnect.groupby("cycle")["size"].sum()
plt.figure(figsize=(10, 4))
bw.plot(kind="line", color="steelblue", marker="o")  # línea con puntos marcados
plt.title("Ancho de banda por ciclo (Interconnect)")
plt.xlabel("Ciclo")
plt.ylabel("Bytes transmitidos")
plt.grid(True)
plt.tight_layout()
plt.show()

# --- Gráfica 2: Tráfico total por PE (enviado y recibido) ---
traffic = []
for i, pe in enumerate(pes):
    sent = pe[pe["recvSend"] == 1]["size"].sum()
    recv = pe[pe["recvSend"] == 0]["size"].sum()
    traffic.append({"PE": f"P{i}", "Enviado": sent, "Recibido": recv})
dfTraffic = pd.DataFrame(traffic).set_index("PE")
dfTraffic.plot(kind="bar", stacked=True, figsize=(8, 4))
plt.title("Tráfico total por PE")
plt.ylabel("Bytes")
plt.tight_layout()
plt.show()

# --- Gráfica 3: Conteo de tipos de mensajes en interconnect ---
msgCounts = interconnect["instr"].value_counts()
plt.figure(figsize=(8, 4))
msgCounts.plot(kind="bar", color="darkorange")
plt.title("Conteo de tipos de mensajes (Interconnect)")
plt.xlabel("Tipo de mensaje")
plt.ylabel("Cantidad")
plt.tight_layout()
plt.show()
