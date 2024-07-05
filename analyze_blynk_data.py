import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns

# Load the CSV file with correct parsing for timestamp
file_path = 'blynk_data.csv'
blynk_data = pd.read_csv(file_path, parse_dates=['timestamp'])

# Define a threshold to filter out timer values that exceed 35,000
threshold = 35000

# Filter out rows where TIMER exceeds the threshold
blynk_data_filtered = blynk_data[blynk_data['TIMER'] < threshold]

# Calculate correlation matrix for the filtered dataset, including luminosity
filtered_correlation_matrix = blynk_data_filtered[['TIMER', 'RSSI INTERNET_NODO', 'RSSI INTERNET_GATEWAY', 'LUMINOSIDADE']].corr()

# Plotting the heatmap for the filtered correlations
plt.figure(figsize=(8, 6))
sns.heatmap(filtered_correlation_matrix, annot=True, cmap='coolwarm', vmin=-1, vmax=1)

plt.title('Filtered Correlation Matrix: RSSI, Timer, and Luminosity')
plt.tight_layout(pad=3.0)
plt.show()

# Plotting the trends of RSSI (both node and gateway), Timer, and Luminosity over time after filtering
plt.figure(figsize=(14, 10))

# RSSI Internet Node over time
plt.subplot(4, 1, 1)
plt.plot(blynk_data_filtered['timestamp'], blynk_data_filtered['RSSI INTERNET_NODO'], label='RSSI Internet Node', color='blue')
plt.xlabel('Time')
plt.ylabel('RSSI (%)')
plt.title('RSSI Internet Node Over Time')
plt.grid(True)

# RSSI Internet Gateway over time
plt.subplot(4, 1, 2)
plt.plot(blynk_data_filtered['timestamp'], blynk_data_filtered['RSSI INTERNET_GATEWAY'], label='RSSI Internet Gateway', color='orange')
plt.xlabel('Time')
plt.ylabel('RSSI (%)')
plt.title('RSSI Internet Gateway Over Time')
plt.grid(True)

# Timer over time
plt.subplot(4, 1, 3)
plt.plot(blynk_data_filtered['timestamp'], blynk_data_filtered['TIMER'], label='Timer', color='green')
plt.xlabel('Time')
plt.ylabel('Timer (ms)')
plt.title('Timer Over Time')
plt.grid(True)

# Luminosity over time
plt.subplot(4, 1, 4)
plt.plot(blynk_data_filtered['timestamp'], blynk_data_filtered['LUMINOSIDADE'], label='Luminosity', color='purple')
plt.xlabel('Time')
plt.ylabel('Luminosity')
plt.title('Luminosity Over Time')
plt.grid(True)

plt.tight_layout(pad=3.0)
plt.show()
