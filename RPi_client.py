import socket
import threading
import time

# Fonction pour se connecter au serveur Arduino
def connect_to_arduino_server(ip, port):
    try:
        # Créer un objet socket
        client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        
        # Se connecter au serveur
        client_socket.connect((ip, port))
        print(f"Connecté au serveur Arduino à {ip}:{port}")

        # Envoyer un message simple au serveur
        message = "I'm here"
        print(message.encode())
        client_socket.sendall(message.encode())

        return client_socket
    except Exception as e:
        print(f"Une erreur s'est produite : {e}")

# Fonction pour écouter le serveur Arduino
def listen_to_arduino_server(client_socket, shared_data, mutex):
    try:
        while True:
            # Recevoir la réponse du serveur
            response = client_socket.recv(1024)
            if not response:
                print("Connexion fermée par le serveur")
                break

            decoded_response = response.decode()
            # print(f"{decoded_response}") # Afficher la réponse du serveur

            # Mettre à jour le dernier message reçu dans la variable partagée
            with mutex:
                shared_data['messages'].append(decoded_response)
                shared_data['is_new_message'] = True # Indiquer qu'il y a de nouveaux messages

            # Envoi d'un accusé de réception en fonction de la réponse du serveur
            if "Still ALIVE" in decoded_response:
                # Extraire le nombre suivant "Still ALIVE"
                still_alive_number = decoded_response.split("Still ALIVE")[1].split(",")[0].strip()
                message = f"Ack {still_alive_number}"
            elif "GPIO" in decoded_response:
                message = f"Ack interrupt on GPIO"
            else:
                message = f"Ack unknown"

            # Envoyer un accusé de réception au serveur
            client_socket.sendall(message.encode())
    except Exception as e:
        print(f"Une erreur s'est produite : {e}")
    finally:
        # Fermer la connexion
        client_socket.close()

# Point d'entrée du programme
if __name__ == "__main__":
    # Paramètres de connexion
    arduino_ip = "192.168.0.3"
    arduino_port = 80

    # Se connecter au serveur Arduino
    my_client_socket = connect_to_arduino_server(arduino_ip, arduino_port)

    # Créer un dictionnaire partagé pour stocker le dernier message
    shared_data = {
        'messages': [],  # Une liste pour stocker les messages reçus
        'is_new_message': False  # Un flag pour indiquer s'il y a un nouveau message
    }

    # Créer un verrou pour synchroniser l'accès aux données partagées
    mutex = threading.Lock()

    # Créer et démarrer un thread pour écouter le serveur
    listener_thread = threading.Thread(target=listen_to_arduino_server, args=(my_client_socket, shared_data, mutex))
    listener_thread.start()

    # Boucle principale pour surveiller les nouveaux messages
    while True:
        with mutex:
            # Si de nouveaux messages ont été reçus
            if shared_data['is_new_message']:
                # Afficher tous les messages reçus
                for msg in shared_data['messages']:
                    print(f"{msg}")

                # Vider la liste après avoir affiché les messages
                shared_data['messages'].clear()
                
                # Réinitialiser le flag
                shared_data['is_new_message'] = False

        # Autres tâches dans la boucle principale
        print(f".")

        # Ajouter un délai pour éviter de consommer 100% du CPU
        time.sleep(0.3)