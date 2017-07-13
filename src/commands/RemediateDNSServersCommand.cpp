using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;
using NLog;
using Synergix.ADCE.Lite.Objects;
using Synergix.ADCE.Lite.Providers;

namespace Synergix.ADCE.Lite.Commands
{
    public class RemediateDNSServersCommand : BaseCommand
    {        
        private readonly DomainControllerProvider _DomainControllerProvider;        
        private readonly NetworkInterfaceProvider _NetworkInterfaceProvider;        

        public RemediateDNSServersCommand(Options options, CommandContext context) : base(options, context)
        {
            Logger = LogManager.GetCurrentClassLogger();
            _DomainControllerProvider = new DomainControllerProvider(options, context);
            _NetworkInterfaceProvider = new NetworkInterfaceProvider(options);
        }

        public bool Execute()
        {
            try
            {
                _NetworkInterfaceProvider.WriteNetworkInformation(Context.NetworkInterface);

                //0. Get current DNS Servers 

                Dictionary<int, IPAddress> currentDnsServers = GetCurrentDNSServers();

                LogCurrentDNSServer(currentDnsServers);

                //1. Get the number of servers we need to find
                int maxServertoFind = 3;

                if (string.Equals("none", Options.DNSServer3, StringComparison.InvariantCultureIgnoreCase))
                {
                    maxServertoFind--;
                }

                Logger.Debug($"Maximum number of DNS servers to discover is {maxServertoFind}");

                var listValidDns = _DomainControllerProvider.GetDomainControllersForDomainByClosesedSite(maxServertoFind);

                if (listValidDns.Count == 0)
                {
                    WriteInformation("No valid or optimal DNS servers are discovered");
                    return true;
                }

                Dictionary<int, DomainController> discoveredDnsServers = SetDiscoveredDNSServers(listValidDns);

                LogDiscoveredServer(discoveredDnsServers);

                if (discoveredDnsServers.Count < maxServertoFind)
                {
                    maxServertoFind = discoveredDnsServers.Count;
                }

                Logger.Debug($"Maximum number of DNS servers to discover after discovery is {maxServertoFind}");

                var allDnsServers = _DomainControllerProvider.AllDiscoveredDomainControllers;

                bool writeDiscoveredServers = false;               

                if (Options.Force)
                {
                    WriteInformation("Program is started with force option; current DNS server configuration will be overriden");
                    writeDiscoveredServers = true;                    
                }
                else
                {
                    if (maxServertoFind == currentDnsServers.Count)
                    {
                        for (int i = 1; i <= maxServertoFind; i++)
                        {
                            var currentDNSIpAddress = currentDnsServers[i];
                            string serverPosition = GetServerName(i);

                            var dnsServer = allDnsServers.FirstOrDefault(x => x.IPAddress.Equals(currentDNSIpAddress.ToString(), StringComparison.InvariantCultureIgnoreCase));
                            string dnsIp = GetIPAddressFromOptions(i);

                            if (dnsServer == null)
                            {
                                WriteError($"Currently configured {serverPosition} DNS server {currentDNSIpAddress} is not valid DNS server, is unreachable or is not from the optimal site configuration");

                                if (string.IsNullOrEmpty(dnsIp))
                                {
                                    writeDiscoveredServers = true;
                                }
                            }
                            else
                            {
                                if (string.IsNullOrEmpty(dnsIp))
                                {
                                    //Auto
                                    if (!dnsServer.SiteName.Equals(discoveredDnsServers[i].SiteName, StringComparison.InvariantCultureIgnoreCase))
                                    {
                                        WriteInformation($"Currently configured {serverPosition} DNS server {currentDNSIpAddress} is not configured from the optimal site. Current site is {dnsServer.SiteName} where expected site is {discoveredDnsServers[i].SiteName}");
                                        writeDiscoveredServers = true;
                                    }
                                    else
                                    {
                                        WriteSuccess($"Currently configured {serverPosition} DNS server {currentDNSIpAddress} is configured from the optimal site {discoveredDnsServers[i].SiteName}");
                                    }
                                }
                                else
                                {
                                    //Manual dns server set
                                    //Check if specified server is in the list
                                    writeDiscoveredServers = true;

                                    var manualDnsServer = allDnsServers.FirstOrDefault(x => x.IPAddress.Equals(dnsIp, StringComparison.InvariantCultureIgnoreCase));
                                    if (manualDnsServer == null)
                                    {
                                        WriteError($"Manualy specified {serverPosition} DNS server {dnsIp} is not valid DNS server, is unreachable or is not from the optimal site configuration");
                                        WriteError($"Regardles of this error we will still configure your provided {serverPosition} DNS server");                                        
                                    }
                                    else
                                    {
                                        if (!manualDnsServer.SiteName.Equals(discoveredDnsServers[i].SiteName, StringComparison.InvariantCultureIgnoreCase))
                                        {
                                            WriteInformation($"Manualy specified {serverPosition} DNS server {dnsIp} is not configured from the optimal site. It's site is {manualDnsServer.SiteName} where expected site is {discoveredDnsServers[i].SiteName}");
                                            WriteInformation($"Regardles of this warning we will still configure your provided {serverPosition} DNS server");
                                        }
                                        else
                                        {
                                            WriteSuccess($"Manualy specified {serverPosition} DNS server {dnsIp} is configured from the optimal site {discoveredDnsServers[i].SiteName}");
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        WriteInformation("Currently configured DNS servers are not optimal becouse the total number of servers is not equal with the expected optimal configuration");
                        writeDiscoveredServers = true;                        
                    }
                }

                Dictionary<int, DomainController> dnsServerList = new Dictionary<int, DomainController>();

                if (writeDiscoveredServers)
                {                    
                    for (int i = 1; i <= maxServertoFind; i++)
                    {
                        string dnsIp = GetIPAddressFromOptions(i);
                        if (string.IsNullOrEmpty(dnsIp))
                        {
                            //auto
                            dnsServerList.Add(i, discoveredDnsServers[i]);
                        }
                        else
                        {
                            //manual - search in the all dc list
                            var manualDnsServer = allDnsServers.FirstOrDefault(x => x.IPAddress.Equals(dnsIp, StringComparison.InvariantCultureIgnoreCase));
                            if (manualDnsServer == null)
                            {
                                dnsServerList.Add(i, new DomainController
                                                     {
                                                         IPAddress = dnsIp
                                                     });
                            }
                            else
                            {
                                dnsServerList.Add(i, manualDnsServer);
                            }
                                
                        }
                    }

                    if (Options.ReadOnlyMode)
                    {
                        Console.WriteLine();
                        WriteSuccess("Suggested optimal DNS Servers");

                        foreach (var dnsServer in dnsServerList)
                        {
                            WriteSuccess(string.IsNullOrEmpty(dnsServer.Value.Name)
                                             ? $"{GetServerName(dnsServer.Key, true)} DNS Server with {dnsServer.Value.IPAddress} from manual entry"
                                             : $"{GetServerName(dnsServer.Key, true)} DNS Server {dnsServer.Value.Name} with {dnsServer.Value.IPAddress} from site {dnsServer.Value.SiteName}");
                        }

                        WriteInformation("Run Application again with UPDATE parameter to perform remediation");
                    }
                    else
                    {
                        Console.WriteLine();
                        WriteSuccess($"Following DNS servers will be commited to network adapter '{Context.NetworkInterface.Name}'");

                        foreach (var dnsServer in dnsServerList)
                        {
                            WriteSuccess(string.IsNullOrEmpty(dnsServer.Value.Name)
                                             ? $"{GetServerName(dnsServer.Key, true)} DNS Server with {dnsServer.Value.IPAddress} from manual entry"
                                             : $"{GetServerName(dnsServer.Key, true)} DNS Server {dnsServer.Value.Name} with {dnsServer.Value.IPAddress} from site {dnsServer.Value.SiteName}");
                        }

                        StringBuilder sb = new StringBuilder();

                        for (int i = 1; i <= maxServertoFind; i++)
                        {
                            sb.Append($",{dnsServerList[i].IPAddress}");
                        }

                        string updateList = sb.ToString().Remove(0, 1);

                        Logger.Debug($"List of DNS that is send to update command '{updateList}'");

                        bool configurationCommited = _NetworkInterfaceProvider.SetDNSServers(Context.NetworkInterface.Id, updateList);

                        if (!configurationCommited)
                        {
                            WriteError("An error occured when commiting the DNS configuration to the network adapter");
                        }

                        return configurationCommited;
                    }
                }
                else
                {
                    WriteSuccess("Current DNS servers configuration is optimal. No changes will be made.");
                }

                return true;
            }
            catch (Exception ex)
            {
                Console.ResetColor();
                Console.WriteLine("Log information is stored in the file: {0}", Options.LogFilePath);

                Logger.LogException(LogLevel.Error, "General error", ex);

                return false;
            }
        }

        private void LogCurrentDNSServer(Dictionary<int, IPAddress> currentDnsServers)
        {
            foreach (var dnsServer in currentDnsServers)
            {
                Logger.Debug($"Discoverd DNS{dnsServer.Key} server IP {dnsServer.Value}");
            }
        }

        private void LogDiscoveredServer(Dictionary<int, DomainController> discoveredDnsServers)
        {
            foreach (var dnsServer in discoveredDnsServers)
            {
                Logger.Debug(string.Format("Discoverd DNS{0} server {3}:{1} from site {2}", dnsServer.Key, dnsServer.Value.IPAddress, dnsServer.Value.SiteName, dnsServer.Value.Name));
            }
        }

        private Dictionary<int, IPAddress> GetCurrentDNSServers()
        {
            var properties = Context.NetworkInterface.GetIPProperties();

            Dictionary<int, IPAddress> servers = new Dictionary<int, IPAddress>();

            int order = 1;
            foreach (IPAddress dnsAddress in properties.DnsAddresses)
            {
                if (dnsAddress.AddressFamily == AddressFamily.InterNetwork)
                {
                    servers.Add(order, dnsAddress);
                    order++;
                }
            }

            return servers;
        }

        private Dictionary<int, DomainController> SetDiscoveredDNSServers(IList<DomainController> discoveredDNSServers)
        {
            Dictionary<int, DomainController> newDnsList = new Dictionary<int, DomainController>();
            int order = 1;

            foreach (var dc in discoveredDNSServers.OrderBy(x => x.Order))
            {
                newDnsList.Add(order, dc);
                order++;
            }

            return newDnsList;
        }

        private string GetIPAddressFromOptions(int i)
        {
            switch (i)
            {
                case 1:
                    return Options.DNSServer1;
                case 2:
                    return Options.DNSServer2;
                case 3:
                    return Options.DNSServer3;
            }

            return string.Empty;
        }

        private string GetServerName(int i, bool capitalize = false)
        {
            switch (i)
            {
                case 1:
                    return capitalize ? "Primary" : "primary";
                case 2:
                    return capitalize ? "Alternate" : "alternate";
                case 3:
                    return capitalize ? "Tertiary" : "tertiary";
            }

            return string.Empty;
        }
    }
}