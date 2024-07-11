import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
from scipy.stats import ttest_ind

# Carregar o arquivo CSV com o parsing correto para timestamp
caminho_arquivo = 'blynk_data.csv'
dados_blynk = pd.read_csv(caminho_arquivo, parse_dates=['timestamp'])

# Remover linhas duplicadas considerando todas as colunas, exceto 'timestamp'
dados_blynk_unicos = dados_blynk.drop_duplicates(subset=dados_blynk.columns.difference(['timestamp']))

# Definir os períodos
inicio_periodo1 = pd.to_datetime('2024-07-11 01:25:10')
fim_periodo1 = pd.to_datetime('2024-07-11 02:25:56')
inicio_periodo2 = pd.to_datetime('2024-07-11 02:52:30')
fim_periodo2 = pd.to_datetime('2024-07-11 03:55:44')

# Filtrar dados para cada período sem quaisquer restrições
dados_periodo1_sem_restricoes = dados_blynk_unicos[(dados_blynk_unicos['timestamp'] >= inicio_periodo1) & (dados_blynk_unicos['timestamp'] <= fim_periodo1)]
dados_periodo2_sem_restricoes = dados_blynk_unicos[(dados_blynk_unicos['timestamp'] >= inicio_periodo2) & (dados_blynk_unicos['timestamp'] <= fim_periodo2)]

# Definir os limites para filtrar valores do TIMER que estão fora da faixa especificada
limite_inferior = 2102
limite_superior = 2112

# Filtrar linhas onde o TIMER está fora dos limites especificados
dados_blynk_filtrados = dados_blynk_unicos[(dados_blynk_unicos['TIMER'] > limite_inferior) & (dados_blynk_unicos['TIMER'] < limite_superior)]

# Filtrar dados para cada período com restrições
dados_periodo1_com_restricoes = dados_blynk_filtrados[(dados_blynk_filtrados['timestamp'] >= inicio_periodo1) & (dados_blynk_filtrados['timestamp'] <= fim_periodo1)]
dados_periodo2_com_restricoes = dados_blynk_filtrados[(dados_blynk_filtrados['timestamp'] >= inicio_periodo2) & (dados_blynk_filtrados['timestamp'] <= fim_periodo2)]

def comparar_periodos(dados1, dados2, titulo_sufixo):
    if not dados1.empty and not dados2.empty:
        # Calcular a média, desvio padrão e moda do TIMER em cada período
        media_timer_periodo1 = dados1['TIMER'].mean()
        desvio_timer_periodo1 = dados1['TIMER'].std()
        moda_timer_periodo1 = dados1['TIMER'].mode().values

        media_timer_periodo2 = dados2['TIMER'].mean()
        desvio_timer_periodo2 = dados2['TIMER'].std()
        moda_timer_periodo2 = dados2['TIMER'].mode().values
        
        # Combinar as médias, desvios padrões e modas em um único DataFrame para comparação fácil
        df_estatisticas = pd.DataFrame({
            'Período': ['Período 1', 'Período 2'],
            'Média': [media_timer_periodo1, media_timer_periodo2],
            'Desvio Padrão': [desvio_timer_periodo1, desvio_timer_periodo2],
            'Moda': [moda_timer_periodo1, moda_timer_periodo2]
        })

        # Mostrar as estatísticas
        print(f"Estatísticas para {titulo_sufixo}:")
        print(df_estatisticas)

        # Realizar o teste t
        estat_t, valor_p = ttest_ind(dados1['TIMER'], dados2['TIMER'])
        print(f"Estatística T: {estat_t}, Valor P: {valor_p}\n")

        # Calcular a matriz de correlação para o conjunto de dados filtrado, incluindo luminosidade
        dados_combinados = pd.concat([dados1, dados2])
        matriz_correlacao_filtrada = dados_combinados[['TIMER', 'RSSI INTERNET_NODO', 'RSSI INTERNET_GATEWAY', 'LUMINOSIDADE']].corr()

        # Plotar a matriz de correlação
        plt.figure(figsize=(10, 8))
        sns.heatmap(matriz_correlacao_filtrada, annot=True, cmap='coolwarm', vmin=-1, vmax=1)

        plt.title(f'Matriz de Correlação ({titulo_sufixo})')
        plt.tight_layout(pad=3.0)
        plt.show()

        # Plotar a comparação de estatísticas
        plt.figure(figsize=(20, 24))

        # Comparação de estatísticas do TIMER
        plt.subplot(3, 2, 1)
        largura_barra = 0.35
        index = df_estatisticas['Período']
        plt.bar(index, df_estatisticas['Média'], largura_barra, label='Média', color='green', yerr=df_estatisticas['Desvio Padrão'], capsize=5)
        plt.xlabel('Período')
        plt.ylabel('TIMER (ms)')
        plt.title(f'Comparação de Média e Desvio Padrão do TIMER ({titulo_sufixo})')
        plt.legend()

        # Gráfico de dispersão do TIMER para cada período
        plt.subplot(3, 2, 2)
        plt.scatter(dados1['timestamp'], dados1['TIMER'], color='blue', label='Período 1')
        plt.scatter(dados2['timestamp'], dados2['TIMER'], color='red', label='Período 2')
        plt.xlabel('Tempo')
        plt.ylabel('TIMER (ms)')
        plt.title(f'Gráfico de Dispersão do TIMER para Cada Período ({titulo_sufixo})')
        plt.legend()

        # Histograma do TIMER para cada período
        plt.subplot(3, 2, 3)
        plt.hist(dados1['TIMER'], bins=20, color='blue', alpha=0.5, label='Período 1')
        plt.hist(dados2['TIMER'], bins=20, color='red', alpha=0.5, label='Período 2')
        plt.xlabel('TIMER (ms)')
        plt.ylabel('Frequência')
        plt.title(f'Histograma do TIMER para Cada Período ({titulo_sufixo})')
        plt.legend()

        # RSSI Internet Nodo ao longo do tempo
        plt.subplot(3, 2, 4)
        plt.plot(dados1['timestamp'], dados1['RSSI INTERNET_NODO'], color='blue', label='Período 1')
        plt.plot(dados2['timestamp'], dados2['RSSI INTERNET_NODO'], color='red', label='Período 2')
        plt.xlabel('Tempo')
        plt.ylabel('RSSI INTERNET NODO')
        plt.title(f'RSSI Internet Nodo ao Longo do Tempo ({titulo_sufixo})')
        plt.legend()

        # RSSI Internet Gateway ao longo do tempo
        plt.subplot(3, 2, 5)
        plt.plot(dados1['timestamp'], dados1['RSSI INTERNET_GATEWAY'], color='blue', label='Período 1')
        plt.plot(dados2['timestamp'], dados2['RSSI INTERNET_GATEWAY'], color='red', label='Período 2')
        plt.xlabel('Tempo')
        plt.ylabel('RSSI INTERNET GATEWAY')
        plt.title(f'RSSI Internet Gateway ao Longo do Tempo ({titulo_sufixo})')
        plt.legend()

        # Luminosidade ao longo do tempo
        plt.subplot(3, 2, 6)
        plt.plot(dados1['timestamp'], dados1['LUMINOSIDADE'], color='blue', label='Período 1')
        plt.plot(dados2['timestamp'], dados2['LUMINOSIDADE'], color='red', label='Período 2')
        plt.xlabel('Tempo')
        plt.ylabel('Luminosidade')
        plt.title(f'Luminosidade ao Longo do Tempo ({titulo_sufixo})')
        plt.legend()

        plt.tight_layout(pad=8.0, h_pad=18.0, w_pad=6.0)
        plt.subplots_adjust(top=0.9, bottom=0.1)
        plt.suptitle(f'Estatística T: {estat_t:.2f}, Valor P: {valor_p:.4f} - Diferença Significativa: {"Sim" if valor_p < 0.05 else "Não"}', y=0.98)
        plt.show()
    else:
        print(f"Sem dados disponíveis para um ou ambos os períodos ({titulo_sufixo}).")

# Comparar períodos sem restrições
comparar_periodos(dados_periodo1_sem_restricoes, dados_periodo2_sem_restricoes, "Sem Restrições")

# Comparar períodos com restrições
comparar_periodos(dados_periodo1_com_restricoes, dados_periodo2_com_restricoes, "Com Restrições")
