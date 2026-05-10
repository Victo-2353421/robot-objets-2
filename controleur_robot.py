import asyncio
from bleak import BleakClient, BleakScanner

# --- Configuration ---
DEVICE_NAME = "Robot6"   # Nom BLE de l'Arduino 

NUS_SERVICE_UUID = "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
RX_CHAR_UUID     = "6e400002-b5a3-f393-e0a9-e50e24dcca9e"  # Central ecrit ici
TX_CHAR_UUID     = "6e400003-b5a3-f393-e0a9-e50e24dcca9e"  # Arduino envoie ici
LUM_CHAR_UUID    = "6e400004-b5a3-f393-e0a9-e50e24dcca9e"  # Notif luminosite

RECONNECT_DELAY = 3.0    # secondes entre les tentatives de reconnexion


async def session(device):
    """Une session de connexion. Retourne True si on doit retenter, False pour quitter."""
    disconnect_event = asyncio.Event()
    connected = [False]
    quit_requested = [False]

    def on_disconnect(_):
        if connected[0]:
            print("\nConnexion BLE perdue.")
            disconnect_event.set()

    def on_tx(_, data):
        msg = data.decode('utf-8', errors='replace').strip()
        print(f"Arduino : {msg}")

    def on_lum(_, data):
        valeur = int.from_bytes(data, byteorder='little', signed=True)
        print(f"Luminosite : {valeur}%")

    try:
        client = BleakClient(device.address, disconnected_callback=on_disconnect)
        await client.connect()
        connected[0] = True
        print("Connecte !")
    except Exception as e:
        print(f"Erreur de connexion : {e}")
        return True  # retenter

    try:
        service_uuids = [s.uuid for s in client.services]
        if NUS_SERVICE_UUID not in service_uuids:
            print("ERREUR : Service NUS introuvable sur ce peripherique.")
            return False

        await client.start_notify(TX_CHAR_UUID, on_tx)
        await client.start_notify(LUM_CHAR_UUID, on_lum)

        print("""
--- Controle Robot Pret ---
  F = Avant            B = Arriere
  L = Lat. gauche      R = Lat. droite
  V = Rot. gauche      X = Rot. droite
  S = Stop
  + = Vitesse +10      - = Vitesse -10
  P = Jouer audio.wav  P:fichier.wav = Jouer un fichier specifique
  D:ON  = Allumer yeux
  D:OFF = Eteindre yeux
  Q = Quitter
---------------------------""")

        loop = asyncio.get_event_loop()

        while not disconnect_event.is_set():
            commande = await loop.run_in_executor(None, input, "Commande : ")
            commande = commande.strip()

            if disconnect_event.is_set():
                break

            if commande.upper() == 'Q':
                print("Deconnexion.")
                quit_requested[0] = True
                break

            if commande:
                try:
                    await client.write_gatt_char(
                        RX_CHAR_UUID,
                        commande.encode('utf-8'),
                        response=False
                    )
                except Exception as e:
                    print(f"Echec d'envoi : {e}")
                    break

    finally:
        try:
            await client.stop_notify(TX_CHAR_UUID)
        except Exception:
            pass
        try:
            await client.stop_notify(LUM_CHAR_UUID)
        except Exception:
            pass
        try:
            await client.disconnect()
        except Exception:
            pass
        print("Deconnecte.")

    return not quit_requested[0]


async def main():
    while True:
        print(f"Scan en cours pour '{DEVICE_NAME}'...")
        device = await BleakScanner.find_device_by_name(DEVICE_NAME, timeout=10.0)

        if device is None:
            print(f"Peripherique '{DEVICE_NAME}' introuvable. Nouvelle tentative dans {RECONNECT_DELAY}s...")
            await asyncio.sleep(RECONNECT_DELAY)
            continue

        print(f"Trouve : {device.name} [{device.address}]")

        retry = await session(device)
        if not retry:
            break

        print(f"Reconnexion dans {RECONNECT_DELAY}s...")
        await asyncio.sleep(RECONNECT_DELAY)


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\nArret demande par l'utilisateur.")
