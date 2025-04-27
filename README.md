# Simulation IoT Wi-Fi avec NS-3

Ce projet simule un réseau IoT utilisant NS-3 avec une communication Wi-Fi. Il inclut la modélisation de la consommation d'énergie et la surveillance du flux pour les dispositifs IoT. La simulation implique plusieurs dispositifs qui communiquent via Wi-Fi sur un réseau 5 GHz.

## Fonctionnalités

- **Dispositifs IoT** : 10 dispositifs dans la simulation (peuvent être personnalisés).
- **Configuration Wi-Fi** : Norme 802.11n, fréquence 5 GHz.
- **Modélisation de la consommation d'énergie** : Source d'énergie et consommation d'énergie radio modélisées pour chaque dispositif.
- **Surveillance des flux** : Suivi des paquets transmis, reçus, du taux de livraison, du débit et du délai.
- **Modèle de mobilité** : Les dispositifs sont placés dans une grille, avec des positions fixes pendant la simulation.
- **Traçage PCAP** : Génère des fichiers `.pcap` pour une analyse plus poussée (si activé).

## Prérequis

- **NS-3** : Un simulateur de réseau événementiel utilisé pour modéliser le réseau IoT.
- **Linux (Kali)** : La simulation est conçue pour fonctionner sur des distributions Linux.
  
### Installation de NS-3

1. Clonez le repository :
   ```bash
   git clone https://github.com/Nelchael2809/iot-wifi-simulation.git
