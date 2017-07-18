#include <iostream>
#include <map>

class RemediateWINSServersCommand : BaseCommand {
private:
    NetworkInterfaceProvider _NetworkInterfaceProvider;
    WINSServerProvider _WINSServerProvider;

public:
    RemediateWINSServersCommand(Options options, CommandContext context) : base(options, context) {
        Logger = LogManager.GetCurrentClassLogger();
        _WINSServerProvider = new WINSServerProvider(options, context);
        _NetworkInterfaceProvider = new NetworkInterfaceProvider(options);
    }

    bool Execute() {
        try {
            _NetworkInterfaceProvider.WriteNetworkInformation(Context.NetworkInterface);

            Dictionary<int, IPAddress> currentWinsServers = GetCurrentWINSServers();

            LogCurrentWINSServer(currentWinsServers);

            int maxServertoFind = 2;

            Logger.Debug($
            "Maximum number of WINS servers to discover is {maxServertoFind}");

            var listValidDns = _WINSServerProvider.GetWINSServersFromClosesedSite(maxServertoFind);

            if (listValidDns.Count == 0) {
                WriteInformation("No valid or optimal WINS servers are discovered");
                return true;
            }

            Dictionary<int, WindowsServer> discoveredWINSServers = SetDiscoveredWINSServers(listValidDns);

            LogDiscoveredServer(discoveredWINSServers);

            if (discoveredWINSServers.Count < maxServertoFind) {
                maxServertoFind = discoveredWINSServers.Count;
            }

            Logger.Debug($
            "Maximum number of WINS servers to discover after discovery is {maxServertoFind}");

            bool writeDiscoveredServers = false;

            if (Options.Force) {
                WriteInformation(
                        "Program is started with force option; current WINS server configuration will be overriden");
                writeDiscoveredServers = true;
            } else {
                if (maxServertoFind == currentWinsServers.Count) {
                    for (int i = 1; i <= maxServertoFind; i++) {
                        var currentWinsServer = currentWinsServers[i];
                        string serverPosition = GetServerName(i);

                        var winsServer = discoveredWINSServers[i];
                        string winsIp = GetIPAddressFromOptions(i);

                        if (winsServer.IPAddress.Equals(currentWinsServer.ToString(),
                                                        StringComparison.InvariantCultureIgnoreCase)) {
                            if (string.IsNullOrEmpty(winsIp)) {
                                //Auto
                                WriteSuccess($
                                "Currently configured {serverPosition} WINS server {currentWinsServer} is configured optimaly");
                            } else {
                                //Manual wins server set
                                //Check if specified server is equal

                                if (winsIp.Equals(winsServer.IPAddress, StringComparison.InvariantCultureIgnoreCase)) {
                                    WriteSuccess($
                                    "Manualy specified {serverPosition} WINS server {winsIp} is configured optimaly");
                                } else {
                                    writeDiscoveredServers = true;
                                    WriteError($
                                    "Manualy specified {serverPosition} WINS server {winsIp} is not configured optimaly");
                                    WriteError($
                                    "Regardles of this error we will still configure your provided {serverPosition} WINS server");
                                }
                            }
                        } else {
                            writeDiscoveredServers = true;
                            WriteError($
                            "Currently configured {serverPosition} WINS server {currentWinsServer} is not configured optimaly");
                        }
                    }
                } else {
                    WriteInformation(
                            "Currently configured WINS servers are not optimal becouse the total number of servers is not equal with the expected optimal configuration");
                    writeDiscoveredServers = true;
                }
            }

            Dictionary<int, WindowsServer> winsServerList = new Dictionary<int, WindowsServer>();

            if (writeDiscoveredServers) {
                for (int i = 1; i <= maxServertoFind; i++) {
                    string dnsIp = GetIPAddressFromOptions(i);
                    if (string.IsNullOrEmpty(dnsIp)) {
                        //auto
                        winsServerList.Add(i, discoveredWINSServers[i]);
                    } else {
                        //manual
                        winsServerList.Add(i, new WindowsServer
                                {
                                        IPAddress = dnsIp
                                });
                    }
                }

                if (Options.ReadOnlyMode) {
                    Console.WriteLine();
                    WriteSuccess("Suggested optimal WINS Servers");

                    foreach(var
                    dnsServer
                            in
                    winsServerList)
                    {
                        WriteSuccess($
                        "{GetServerName(dnsServer.Key, true)} WINS Server with {dnsServer.Value.IPAddress}");
                    }

                    WriteInformation("Run Application again with UPDATE parameter to perform remediation");
                } else {
                    Console.WriteLine();
                    WriteSuccess($
                    "Following WINS servers will be commited to network adapter '{Context.NetworkInterface.Name}'");

                    foreach(var
                    dnsServer
                            in
                    winsServerList)
                    {
                        WriteSuccess($
                        "{GetServerName(dnsServer.Key, true)} WINS Server with {dnsServer.Value.IPAddress}");
                    }

                    string wins1 = string.Empty;
                    string wins2 = string.Empty;

                    for (int i = 1; i <= maxServertoFind; i++) {
                        if (i == 1) {
                            wins1 = winsServerList[i].IPAddress;
                        }

                        if (i == 2) {
                            wins2 = winsServerList[i].IPAddress;
                        }
                    }

                    bool configurationCommited = _NetworkInterfaceProvider.SetWINSServers(Context.NetworkInterface.Id,
                                                                                          wins1, wins2);

                    if (!configurationCommited) {
                        WriteError("An error occured when commiting the WINS configuration to the network adapter");
                    }

                    return configurationCommited;
                }
            } else {
                WriteSuccess("Current WINS servers configuration is optimal. No changes will be made.");
            }

            return true;
        }
        catch (Exception ex) {
            Console.ResetColor();
            Console.WriteLine("Log information is stored in the file: {0}", Options.LogFilePath);

            Logger.LogException(LogLevel.Error, "General error", ex);

            return false;
        }
    }

private:
    Dictionary<int, IPAddress> GetCurrentWINSServers() {
        ManagementClass objMc = new ManagementClass("Win32_NetworkAdapterConfiguration");
        ManagementObjectCollection objMoc = objMc.GetInstances();

        foreach(ManagementObject
        objMo
                in
        objMoc)
        {
            if (objMo["SettingID"].Equals(Context.NetworkInterface)) {
                Dictionary<int, IPAddress> servers = new Dictionary<int, IPAddress>();

                try {
                    if (objMo["WINSPrimaryServer"] != null) {
                        IPAddress w1;

                        if (IPAddress.TryParse(objMo["WINSPrimaryServer"].ToString(), out w1))
                        {
                            servers.Add(1, w1);
                        }
                    }

                    if (objMo["WINSSecondaryServer"] != null) {
                        IPAddress w2;

                        if (IPAddress.TryParse(objMo["WINSSecondaryServer"].ToString(), out w2))
                        {
                            servers.Add(2, w2);
                        }
                    }
                }
                catch (Exception ex) {
                    Logger.LogException(LogLevel.Error, "SetDNSServers", ex);
                }

                return servers;
            }
        }

        return new Dictionary<int, IPAddress>();
    }

    void LogCurrentWINSServer(Dictionary<int, IPAddress> currentDnsServers) {
        foreach(var
        dnsServer
                in
        currentDnsServers)
        {
            Logger.Debug($
            "Discoverd WINS{dnsServer.Key} server IP {dnsServer.Value}");
        }
    }

    Dictionary<int, WindowsServer> SetDiscoveredWINSServers(IList <WindowsServer> discoveredWinsServers) {
        Dictionary<int, WindowsServer> newDnsList = new Dictionary<int, WindowsServer>();
        int order = 1;

        foreach(var
        dc
                in
        discoveredWinsServers.OrderBy(x = > x.Order))
        {
            newDnsList.Add(order, dc);
            order++;
        }

        return newDnsList;
    }

    void LogDiscoveredServer(Dictionary<int, WindowsServer> discoveredWinsServers) {
        foreach(var
        winsServer
                in
        discoveredWinsServers)
        {
            Logger.Debug(string.Format("Discoverd WINS{0} server {3}:{1} from site {2}", winsServer.Key,
                                       winsServer.Value.IPAddress, winsServer.Value.SiteName, winsServer.Value.Name));
        }
    }

    string GetIPAddressFromOptions(int i) {
        switch (i) {
            case 1:
                return Options.WINSServer1;
            case 2:
                return Options.WINSServer2;
        }

        return string.Empty;
    }

    string GetServerName(int i, bool capitalize = false) {
        switch (i) {
            case 1:
                return capitalize ? "Primary" : "primary";
            case 2:
                return capitalize ? "Alternate" : "alternate";
            case 3:
                return capitalize ? "Tertiary" : "tertiary";
        }

        return string.Empty;
    }
};