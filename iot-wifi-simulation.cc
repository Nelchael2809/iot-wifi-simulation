#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/energy-module.h"
#include "ns3/mobility-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;
using namespace ns3::energy; // Ajouté pour les classes d'énergie

NS_LOG_COMPONENT_DEFINE("IoTWiFiSimulation");

int main(int argc, char *argv[])
{
    // Paramètres configurables avec valeurs par défaut
    uint32_t nDevices = 10;
    double simulationTime = 60.0;
    bool enablePcap = true; // Activez le traçage PCAP pour diagnostiquer
    double txPower = 20.0; // Augmentez la puissance de transmission pour améliorer la portée
    double initialEnergy = 10000.0; // en Joules

    CommandLine cmd(__FILE__);
    cmd.AddValue("nDevices", "Number of IoT devices", nDevices);
    cmd.AddValue("simulationTime", "Simulation time in seconds", simulationTime);
    cmd.AddValue("enablePcap", "Enable PCAP tracing", enablePcap);
    cmd.AddValue("txPower", "Transmission power in dBm", txPower);
    cmd.AddValue("initialEnergy", "Initial energy in Joules", initialEnergy);
    cmd.Parse(argc, argv);

    // Configurer les paramètres Wi-Fi
    Config::SetDefault("ns3::WifiRemoteStationManager::NonUnicastMode",
                      StringValue("OfdmRate6Mbps"));

    // Créer les noeuds IoT
    NodeContainer iotNodes;
    iotNodes.Create(nDevices);

    // Configuration Wi-Fi
    WifiHelper wifi;
    wifi.SetStandard(ns3::WIFI_STANDARD_80211ac); // Correction ici

    // Configuration du MAC
    WifiMacHelper mac;
    mac.SetType("ns3::ApWifiMac",
               "Ssid", SsidValue(Ssid("ns-3-ssid")),
               "BeaconGeneration", BooleanValue(true),
               "BeaconInterval", TimeValue(MilliSeconds(2560))); // 2.56 secondes

    // Modifiez votre configuration du canal comme suit :
    YansWifiChannelHelper channel;
    channel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    channel.AddPropagationLoss("ns3::LogDistancePropagationLossModel",
                             "Exponent", DoubleValue(3.0),
                             "ReferenceLoss", DoubleValue(46.7)); // Pour 5 GHz
    // Configuration du PHY
    YansWifiPhyHelper phy; // Correction ici
    phy.SetChannel(channel.Create());
  


    // Configurer les paramètres de transmission
    phy.Set("TxPowerStart", DoubleValue(txPower));
    phy.Set("TxPowerEnd", DoubleValue(txPower));
    phy.Set("TxPowerLevels", UintegerValue(1));
    phy.Set("RxGain", DoubleValue(0));
    phy.Set("TxGain", DoubleValue(0));
    phy.Set("RxNoiseFigure", DoubleValue(7));

    // Installer le dispositif sur le noeud AP (index 0)
    NetDeviceContainer apDevice = wifi.Install(phy, mac, iotNodes.Get(0));

    // Configurer les stations (autres noeuds)
    mac.SetType("ns3::StaWifiMac",
               "Ssid", SsidValue(Ssid("ns-3-ssid")),
               "ActiveProbing", BooleanValue(false));

    NetDeviceContainer staDevices;
    for (uint32_t i = 1; i < nDevices; ++i) {
        staDevices.Add(wifi.Install(phy, mac, iotNodes.Get(i)));
    }

    // Combiner tous les dispositifs
    NetDeviceContainer devices;
    devices.Add(apDevice);
    devices.Add(staDevices);

    // Configuration de la mobilité
    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                "MinX", DoubleValue(0.0),
                                "MinY", DoubleValue(0.0),
                                "DeltaX", DoubleValue(2.0),
                                "DeltaY", DoubleValue(2.0),
                                "GridWidth", UintegerValue(3),
                                "LayoutType", StringValue("RowFirst"));
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(iotNodes);

    // Installer la pile Internet
    InternetStackHelper stack;
    stack.Install(iotNodes);

    // Assigner les adresses IP
    Ipv4AddressHelper address;
    address.SetBase("192.168.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // Configuration du modèle d'énergie
    BasicEnergySourceHelper energySourceHelper;
    energySourceHelper.Set("BasicEnergySourceInitialEnergyJ", DoubleValue(initialEnergy));
    EnergySourceContainer energySources = energySourceHelper.Install(iotNodes);

    WifiRadioEnergyModelHelper radioEnergyHelper;
    radioEnergyHelper.Set("TxCurrentA", DoubleValue(0.0174));  // 17.4 mA
    radioEnergyHelper.Set("RxCurrentA", DoubleValue(0.0197));  // 19.7 mA
    radioEnergyHelper.Set("IdleCurrentA", DoubleValue(0.0006)); // 0.6 mA
    DeviceEnergyModelContainer energyModels = radioEnergyHelper.Install(devices, energySources);

    // Applications IoT
    // Serveur Echo UDP sur le noeud AP (index 0)
    UdpEchoServerHelper echoServer(9); // Port standard pour les tests
    ApplicationContainer serverApps = echoServer.Install(iotNodes.Get(0));
    serverApps.Start(Seconds(0.0));
    serverApps.Stop(Seconds(simulationTime));

    // Clients Echo UDP sur les autres noeuds
    UdpEchoClientHelper echoClient(interfaces.GetAddress(0), 9);
    echoClient.SetAttribute("MaxPackets", UintegerValue(100));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(5.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(100));

    ApplicationContainer clientApps;
    for (uint32_t i = 1; i < nDevices; ++i) {
        clientApps.Add(echoClient.Install(iotNodes.Get(i)));
    }
    clientApps.Start(Seconds(1.0));
    clientApps.Stop(Seconds(simulationTime - 1));

    // Activer le traçage PCAP si demandé
    if (enablePcap) {
        phy.EnablePcap("iot-wifi", devices);
    }

    // Configurer le monitoring des flux
    FlowMonitorHelper flowMonitor;
    Ptr<FlowMonitor> monitor = flowMonitor.InstallAll();

    // Lancer la simulation
    Simulator::Stop(Seconds(simulationTime));
    Simulator::Run();

    // Analyse des résultats
    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowMonitor.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();

    uint64_t totalTxPackets = 0;
    uint64_t totalRxPackets = 0;
    double totalDelay = 0.0;
    uint64_t totalRxBytes = 0;

    for (auto it = stats.begin(); it != stats.end(); ++it) {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(it->first);

        std::cout << "\nFlow " << it->first << " (" << t.sourceAddress << " -> "
                  << t.destinationAddress << ")\n";
        std::cout << "  Tx Packets: " << it->second.txPackets << "\n";
        std::cout << "  Rx Packets: " << it->second.rxPackets << "\n";

        if (it->second.txPackets > 0) {
            double deliveryRatio = (it->second.rxPackets * 100.0) / it->second.txPackets;
            std::cout << "  Packet delivery ratio: " << deliveryRatio << "%\n";

            if (it->second.rxPackets > 0) {
                double avgDelay = it->second.delaySum.GetSeconds() / it->second.rxPackets;
                std::cout << "  Average delay: " << avgDelay << "s\n";
                totalDelay += avgDelay;
            }
        }

        double throughput = (it->second.rxBytes * 8.0) / (it->second.timeLastRxPacket.GetSeconds() - it->second.timeFirstTxPacket.GetSeconds()) / 1000;
        std::cout << "  Throughput: " << throughput << " kbps\n";

        totalTxPackets += it->second.txPackets;
        totalRxPackets += it->second.rxPackets;
        totalRxBytes += it->second.rxBytes;
    }

    // Calculer la consommation d'énergie
    double totalEnergyConsumed = 0.0;
    for (DeviceEnergyModelContainer::Iterator iter = energyModels.Begin(); iter != energyModels.End(); iter++) {
        totalEnergyConsumed += (*iter)->GetTotalEnergyConsumption();
    }

    // Afficher les statistiques globales
    std::cout << "\nRésultats globaux de la simulation:\n";
    std::cout << "--------------------------------\n";
    std::cout << "Nombre de dispositifs IoT: " << nDevices << "\n";
    std::cout << "Durée de simulation: " << simulationTime << "s\n";
    std::cout << "Paquets transmis totaux: " << totalTxPackets << "\n";
    std::cout << "Paquets reçus totaux: " << totalRxPackets << "\n";

    if (totalTxPackets > 0) {
        std::cout << "Taux de livraison global: " << (totalRxPackets * 100.0 / totalTxPackets) << "%\n";
    }

    if (totalRxPackets > 0) {
        std::cout << "Délai moyen global: " << (totalDelay / stats.size()) << "s\n";
    }

    std::cout << "Débit global: " << (totalRxBytes * 8.0 / simulationTime / 1000) << " kbps\n";
    std::cout << "Consommation énergétique totale: " << totalEnergyConsumed << " J\n";
    std::cout << "Consommation moyenne par dispositif: " << (totalEnergyConsumed / nDevices) << " J\n";

    Simulator::Destroy();
    return 0;
}

