import numpy as np
import matplotlib.pyplot as plt

# Carregar os dados do arquivo CSV
data = np.loadtxt('dadosMPU.csv', delimiter=',', skiprows=1)

# Extrair as colunas
numero_amostra = data[:, 0]  # Primeira coluna: número da amostra (usada como proxy para tempo)
accel_x = data[:, 1]         # Aceleração no eixo X
accel_y = data[:, 2]         # Aceleração no eixo Y
accel_z = data[:, 3]         # Aceleração no eixo Z
giro_x = data[:, 4]          # Giroscópio no eixo X
giro_y = data[:, 5]          # Giroscópio no eixo Y
giro_z = data[:, 6]          # Giroscópio no eixo Z

# Plotar os dados de aceleração
plt.figure(figsize=(10, 6))
plt.plot(numero_amostra, accel_x, label='Accel X', color='r')
plt.plot(numero_amostra, accel_y, label='Accel Y', color='g')
plt.plot(numero_amostra, accel_z, label='Accel Z', color='b')
plt.title("Aceleração")
plt.xlabel("Tempo (Número da Amostra)")
plt.ylabel("Aceleração")
plt.legend()
plt.grid()
plt.show()

# Plotar os dados de giroscópio
plt.figure(figsize=(10, 6))
plt.plot(numero_amostra, giro_x, label='Giro X', color='r')
plt.plot(numero_amostra, giro_y, label='Giro Y', color='g')
plt.plot(numero_amostra, giro_z, label='Giro Z', color='b')
plt.title("Giroscópio")
plt.xlabel("Tempo (Número da Amostra)")
plt.ylabel("Giroscópio")
plt.legend()
plt.grid()
plt.show()