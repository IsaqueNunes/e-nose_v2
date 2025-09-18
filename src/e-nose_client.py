import asyncio
import argparse
import pandas as pd
from datetime import datetime
from bleak import BleakClient, BleakScanner
import struct
import os

DEVICE_NAME = "E-Nose"
SERVICE_UUID = "bea5692f-939d-4e5a-bfa9-80d3efb8e3cb"
CHARACTERISTIC_UUID = "b13493c7-5499-4b0a-a3d9-66eea53f382c"

# --- Estrutura de Desempacotamento ---
# Isso define como os 44 bytes recebidos serão interpretados.
# '<' indica little-endian (padrão do ESP32)
# f = float (4 bytes)
# H = unsigned short (2 bytes)
# i = int (4 bytes)
# l = long (4 bytes)
# 6x floats, 4x uint16_t, 1x float, 1x int, 1x long
# DATA_FORMAT_STRING = '<ffffffHHHHfil'
DATA_FORMAT_STRING = '<fffffffffffil'

# --- Nomes das Colunas para o DataFrame/CSV ---
COLUMN_NAMES = [
    'Timestamp', 'BME_Temp', 'BME_Hum', 'BME_Pres', 'BME_Gas', 'SHT_Temp', 'SHT_Hum',
    'MQ3', 'MQ135', 'MQ136', 'MQ137', 'ADC_RMS', 'ADC_Channel', 'ADC_Frequency'
]

def notification_handler(sender, data: bytearray):
    """
    Callback executado toda vez que uma notificação BLE é recebida.
    Desempacota os dados, cria uma linha no DataFrame e anexa ao CSV.
    """
    global output_csv_path

    try:
        # 1. Desempacota os bytes recebidos usando a string de formato
        unpacked_data = struct.unpack(DATA_FORMAT_STRING, data)

        # 2. Adiciona um timestamp ao início dos dados
        timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f')
        full_data_row = [timestamp] + list(unpacked_data)

        # 3. Cria um DataFrame de uma única linha com os dados
        df_new_row = pd.DataFrame([full_data_row], columns=COLUMN_NAMES)

        print(f"Received data: ADC_RMS={unpacked_data[10]:.2f}, Freq={unpacked_data[12]}")

        # 4. Anexa ao arquivo CSV
        # 'a' para append, header=False para não reescrever o cabeçalho
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

    # Cria o arquivo CSV e escreve o cabeçalho se ele não existir
    if not os.path.exists(output_csv_path):
        pd.DataFrame(columns=COLUMN_NAMES).to_csv(output_csv_path, index=False)
        print(f"Created new CSV file: {output_csv_path}")
    else:
        print(f"Appending to existing CSV file: {output_csv_path}")

    # Callback para ser notificado sobre desconexões
    def handle_disconnect(client: BleakClient):
        print(f"Device {client.address} disconnected. Attempting to reconnect...")
        # A lógica de reconexão está no loop principal abaixo.
        # Este callback é apenas para notificação.

    # Loop principal para garantir que o script tente se reconectar sempre
    while True:
        device = None
        try:
            print(f"Scanning for device: '{DEVICE_NAME}'...")
            device = await BleakScanner.find_device_by_name(DEVICE_NAME, timeout=10.0)

            if not device:
                print(f"Could not find device '{DEVICE_NAME}'. Retrying in 5 seconds...")
                await asyncio.sleep(5)
                continue # Volta para o início do loop e tenta escanear novamente

            print(f"Found device: {device.name} ({device.address}). Connecting...")

            # O 'async with' gerencia a conexão e desconexão de forma limpa
            async with BleakClient(device, disconnected_callback=handle_disconnect) as client:
                if client.is_connected:
                    print("Connected successfully!")
                    await client.start_notify(CHARACTERISTIC_UUID, notification_handler)
                    print("Notifications started. Waiting for data... (Press Ctrl+C to stop)")

                    # Mantém a conexão ativa enquanto o dispositivo estiver conectado
                    while client.is_connected:
                        await asyncio.sleep(1) # Verifica a cada segundo

        except Exception as e:
            print(f"An error occurred: {e}. Cleaning up and retrying in 5 seconds...")
            # Não é preciso fazer nada com o 'client', o 'async with' já cuida disso.

        # Se saiu do try/except (por erro ou desconexão), espera um pouco antes de tentar de novo
        await asyncio.sleep(5)

# async def main(args):
#     """
#     Função principal assíncrona.
#     Procura pelo dispositivo, conecta, ativa notificações e espera.
#     """
#     global output_csv_path
#     output_csv_path = args.output

#     # Cria o arquivo CSV e escreve o cabeçalho se ele não existir
#     if not os.path.exists(output_csv_path):
#         pd.DataFrame(columns=COLUMN_NAMES).to_csv(output_csv_path, index=False)
#         print(f"Created new CSV file: {output_csv_path}")
#     else:
#         print(f"Appending to existing CSV file: {output_csv_path}")

#     print(f"Scanning for device: '{DEVICE_NAME}'...")
#     device = await BleakScanner.find_device_by_name(DEVICE_NAME, timeout=20.0)

#     if not device:
#         print(f"Could not find device with name '{DEVICE_NAME}'. Please check if it's on and advertising.")
#         return

#     print(f"Connecting to {device.name} ({device.address})...")

#     async with BleakClient(device) as client:
#         if client.is_connected:
#             print(f"Connected successfully!")
#             try:
#                 await client.start_notify(CHARACTERISTIC_UUID, notification_handler)
#                 print("Notifications started. Waiting for data... (Press Ctrl+C to stop)")
#                 # Mantém o script rodando para receber notificações
#                 while True:
#                     await asyncio.sleep(1)
#             except Exception as e:
#                 print(f"An error occurred: {e}")
#             finally:
#                 # Garante que as notificações sejam paradas ao sair
#                 await client.stop_notify(CHARACTERISTIC_UUID)
#                 print("Notifications stopped.")
#         else:
#             print("Failed to connect.")

"""
python /home/isaque/Programming/e-nose_v2/src/e-nose_client.py \
    -o /home/isaque/Programming/e-nose_v2/data/e-nose_data_3.csv
"""

if __name__ == "__main__":
    # Configura o parser de argumentos da linha de comando
    parser = argparse.ArgumentParser(description="BLE Data Collector for E-Nose project.")

    # Gera um nome de arquivo padrão com a data e hora atuais
    default_filename = f"e-nose_data_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"

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
