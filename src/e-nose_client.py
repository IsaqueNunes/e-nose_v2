import asyncio
import argparse
import pandas as pd
from datetime import datetime
from bleak import BleakClient, BleakScanner
import struct
import os

# --- Configurações do Dispositivo e Serviço ---
# ATUALIZADO: O nome do dispositivo foi alterado no ESP32
DEVICE_NAME = "E-Nose_V2_LockIn"
SERVICE_UUID = "bea5692f-939d-4e5a-bfa9-80d3efb8e3cb"
CHARACTERISTIC_UUID = "b13493c7-5499-4b0a-a3d9-66eea53f382c"

# --- Configuração da Estrutura de Dados (DEVE CORRESPONDER AO ESP32) ---
NUM_FREQUENCIAS = 6
NUM_CANAIS = 4
ADC_DATA_POINTS = NUM_FREQUENCIAS * NUM_CANAIS

# ATUALIZADO: A nova estrutura de dados consiste em:
# 10 floats (sensores comerciais) + 16 floats (adc_mean) + 16 floats (adc_std_dev) = 42 floats
NUM_FLOATS = 10 + (2 * ADC_DATA_POINTS)
DATA_FORMAT_STRING = f'<{NUM_FLOATS}f'
EXPECTED_DATA_SIZE = struct.calcsize(DATA_FORMAT_STRING) # Calcula o tamanho esperado em bytes

# --- Geração dos Nomes das Colunas para o CSV ---
# Frequências usadas no ESP32 para nomear as colunas corretamente
FREQUENCIES_HZ = [100, 1000, 5000, 10000, 50000, 100000]

# Colunas fixas
COLUMN_NAMES = [
    'Timestamp', 'BME_Temp', 'BME_Hum', 'BME_Pres', 'BME_Gas',
    'SHT_Temp', 'SHT_Hum', 'MQ3', 'MQ135', 'MQ136', 'MQ137'
]

# Adiciona dinamicamente as colunas do sensor fabricado
adc_columns = []
for i in range(NUM_FREQUENCIAS):
    for j in range(NUM_CANAIS):
        freq = FREQUENCIES_HZ[i]
        ch = j + 1
        adc_columns.append(f'Ch{ch}_F{freq}Hz_Mean')
        adc_columns.append(f'Ch{ch}_F{freq}Hz_StdDev')

# Combina as colunas de Média e Desvio Padrão
# O desempacotamento retornará todos os 'mean' primeiro, depois todos os 'std_dev'
mean_columns = [col for col in adc_columns if 'Mean' in col]
std_dev_columns = [col for col in adc_columns if 'StdDev' in col]

# Junta tudo na ordem correta
COLUMN_NAMES.extend(mean_columns + std_dev_columns)


def notification_handler(sender, data: bytearray):
    """
    Callback executado toda vez que uma notificação BLE é recebida.
    Desempacota os dados, cria uma linha no DataFrame e anexa ao CSV.
    """
    global output_csv_path

    # Verifica se o tamanho dos dados recebidos corresponde ao esperado
    if len(data) != EXPECTED_DATA_SIZE:
        print(f"Error: Received {len(data)} bytes, but expected {EXPECTED_DATA_SIZE}. Skipping packet.")
        return

    try:
        # 1. Desempacota os bytes recebidos usando a string de formato
        unpacked_data = struct.unpack(DATA_FORMAT_STRING, data)

        # 2. Adiciona um timestamp ao início dos dados
        timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f')
        full_data_row = [timestamp] + list(unpacked_data)

        # 3. Cria um DataFrame de uma única linha com os dados
        df_new_row = pd.DataFrame([full_data_row], columns=COLUMN_NAMES)

        # Exibe um resumo dos dados recebidos para feedback
        first_adc_mean = unpacked_data[10] # O primeiro valor de 'adc_mean'
        print(f"Received data packet at {timestamp}. First ADC Mean: {first_adc_mean:.4f}")

        # 4. Anexa ao arquivo CSV
        df_new_row.to_csv(output_csv_path, mode='a', header=False, index=False)

    except struct.error as e:
        print(f"Error unpacking data: {e}. Received {len(data)} bytes.")
    except Exception as e:
        print(f"An error occurred in notification_handler: {e}")


async def main(args):
    """
    Função principal assíncrona.
    Procura pelo dispositivo, conecta, ativa notificações e gerencia reconexões.
    """
    global output_csv_path
    output_csv_path = args.output

    if not os.path.exists(output_csv_path):
        pd.DataFrame(columns=COLUMN_NAMES).to_csv(output_csv_path, index=False)
        print(f"Created new CSV file: {output_csv_path}")
    else:
        print(f"Appending to existing CSV file: {output_csv_path}")

    def handle_disconnect(client: BleakClient):
        print(f"Device {client.address} disconnected. Attempting to reconnect...")

    while True:
        device = None
        try:
            print(f"Scanning for device: '{DEVICE_NAME}'...")
            device = await BleakScanner.find_device_by_name(DEVICE_NAME, timeout=10.0)

            if not device:
                print(f"Could not find device '{DEVICE_NAME}'. Retrying in 5 seconds...")
                await asyncio.sleep(5)
                continue

            print(f"Found device: {device.name} ({device.address}). Connecting...")

            async with BleakClient(device, disconnected_callback=handle_disconnect) as client:
                if client.is_connected:
                    print("Connected successfully!")
                    await client.start_notify(CHARACTERISTIC_UUID, notification_handler)
                    print("Notifications started. Waiting for data... (Press Ctrl+C to stop)")

                    while client.is_connected:
                        await asyncio.sleep(1)

        except Exception as e:
            print(f"An error occurred: {e}. Cleaning up and retrying in 5 seconds...")

        await asyncio.sleep(5)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="BLE Data Collector for E-Nose project (Lock-In Version).")
    default_filename = f"e-nose_lockin_data_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"
    parser.add_argument(
        "-o", "--output",
        type=str,
        default=default_filename,
        help=f"Output CSV file name. Default: {default_filename}"
    )
    args = parser.parse_args()

    try:
        asyncio.run(main(args))
    except KeyboardInterrupt:
        print("\nProgram interrupted by user. Exiting.")
