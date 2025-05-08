# Simulador de Interconexión entre Procesadores

Este proyecto simula un sistema de interconexión entre 8 procesadores con soporte para ejecución en modo FIFO o por prioridad, y carga de distintos sets de instrucciones (tests).

## 🔧 Instrucciones para Compilar y Ejecutar

### 1. Ir a la Carpeta `build`

Abrí una terminal en la carpeta raíz del proyecto y ejecutá:

```bash
cd build
```

### Construir el Ejecutable

Ejecutá el siguiente comando para compilar el proyecto:
```bash
make
```

### 3. Ejecutar el Simulador
```bash
./Interconnect_A2 [modo_ejecucion] [test_usado]
```

Parámetros:

    modo_ejecucion:

        0 → Modo FIFO

        1 → Modo por Prioridad

    test_usado:

        1 o 2, dependiendo del set de instrucciones que se desea probar (deben estar cargados previamente en la carpeta correspondiente).

### Ejemplo de ejecución:
```bash
./Interconnect_A2 0 1
```


Esto ejecuta el simulador en modo FIFO con el conjunto de instrucciones del test 1.

📁 Los archivos de salida se guardan en la carpeta output.


---
