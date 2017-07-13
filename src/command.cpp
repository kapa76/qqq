using System;
using System.Collections.Generic;
using System.Net;
using System.Net.NetworkInformation;
using System.Net.Sockets;
using System.Threading;
using Microsoft.Win32;
using NLog;
using Synergix.ADCE.Lite.Core;
using Synergix.ADCE.Lite.Objects;

namespace Synergix.ADCE.Lite
{
    public class Command
    {
        private static readonly Logger Logger = LogManager.GetCurrentClassLogger();
        private static readonly string LoopbackIPv4Address = "127.0.0.1";

        public static bool Execute()
        {
            try
            {
                var listValidDns = DnsDiscovery.GetList();

                CheckDnsForNic(GlobalData.NetworkInterface, listValidDns);

                return true;
            }
            catch (Exception ex)
            {
                Console.ResetColor();
                Console.WriteLine("Log information is stored in the file: {0}", GlobalData.LogFilePath);

                Logger.LogException(LogLevel.Error, "General error", ex);

                return false;
            }
        }

        private static void CheckDnsForNic(NetworkInterface nic, List<ValidDNS> listValidDns)
        {
            if (!GlobalData.ReadOnly)
            {
                Console.ResetColor();
                Console.WriteLine();
                Console.WriteLine("Information:\tProgram started in UPDATE mode. Optimal DNS Client configuration will be applied.");

                Logger.Warn("Program started in UPDATE mode. Optimal DNS Client configuration will be applied.");
            }
            else
            {
                Console.ResetColor();
                Console.WriteLine();
                Console.WriteLine("Information:\tProgram started in READONLY mode. No changes will be made.");

                Logger.Info("Program started in READONLY mode. No changes will be made.");
            }

            LogNicConfiguration(nic);

            var suggestedNewDnsList = false;

            var properties = nic.GetIPProperties();

            //var ip4Properties = properties.GetIPv4Properties();
            //LINQ
            //var ipv4DNSAddresses = Array.FindAll(properties.DnsAddresses.ToArray(), a => a.AddressFamily == AddressFamily.InterNetwork);

            List<IPAddress> ipv4DNSAddresses = new List<IPAddress>();

            foreach (var dnsAddress in properties.DnsAddresses)
            {
                if (dnsAddress.AddressFamily == AddressFamily.InterNetwork)
                {
                    ipv4DNSAddresses.Add(dnsAddress);
                }
            }

            string sValidDnsListRemediate;
            bool isOptimalDns;


            //LINQ
            //var nicIpAddress = properties.UnicastAddresses.Where(adr => !IPAddress.IsLoopback(adr.Address))
            //        .FirstOrDefault(adr => adr.Address.AddressFamily == AddressFamily.InterNetwork).Address.ToString();

            string nicIpAddress = string.Empty;
            foreach (var adr in properties.UnicastAddresses)
            {
                if (!IPAddress.IsLoopback(adr.Address) && (adr.Address.AddressFamily == AddressFamily.InterNetwork))
                {
                    nicIpAddress = adr.Address.ToString();
                    break;
                }
            }

            var optList = OptimalDnsListNoLoadBalancing(listValidDns, out sValidDnsListRemediate, nicIpAddress);
            var dnsConfiguredOnNIc = new List<ValidDNS>();

            for (var i = 0; i < GlobalData.MaximumDNSServers; i++)
            {
                var validDns = new ValidDNS();
                validDns.IsValid = true;

                try
                {
                    validDns.IpAddress = ipv4DNSAddresses[i].ToString();
                }
                catch (Exception)
                {
                    switch (i)
                    {
                        case 0:

                            Console.ForegroundColor = ConsoleColor.Red;
                            Console.WriteLine("Warning:\tPrimary DNS is not configured");

                            Logger.Warn("Primary DNS is not configured");

                            break;

                        case 1:
                            Console.ForegroundColor = ConsoleColor.Red;
                            Console.WriteLine("Warning:\tAlternate DNS is not configured");

                            Logger.Warn("Alternate DNS is not configured");


                            if (!GlobalData.AlternateDnsAuto)
                            {
                                var tempSplitedServers = new List<string>();
                                var tempSplitedServersArray = sValidDnsListRemediate.Split(',');
                                tempSplitedServers.AddRange(tempSplitedServersArray);

                                if (optList.Count >= i + 1)
                                {
                                    if (!tempSplitedServers[i].Equals(GlobalData.AlternateDnsIp))
                                    {
                                        tempSplitedServers[i] = GlobalData.AlternateDnsIp;
                                    }
                                }
                                else
                                {
                                    var newDNS = new ValidDNS();
                                    newDNS.IpAddress = GlobalData.AlternateDnsIp;

                                    optList.Add(new[] {newDNS});

                                    tempSplitedServers.Add(GlobalData.AlternateDnsIp);
                                }

                                sValidDnsListRemediate = string.Join(",", tempSplitedServers.ToArray());
                            }
                            break;

                        case 2:

                            Console.ResetColor();
                            Console.ForegroundColor = ConsoleColor.Yellow;
                            Console.WriteLine("Warning:\tTertiary DNS is not configured");

                            Logger.Warn("Tertiary DNS is not configured");

                            if (!GlobalData.TerciaryDnsAuto)
                            {
                                var tempSplitedServers = new List<string>();
                                var tempSplitedServersArray = sValidDnsListRemediate.Split(',');
                                tempSplitedServers.AddRange(tempSplitedServersArray);

                                if (optList.Count >= i + 1)
                                {
                                    if (!tempSplitedServers[i].Equals(GlobalData.TerciaryDnsIp))
                                    {
                                        tempSplitedServers[i] = GlobalData.TerciaryDnsIp;
                                    }
                                }
                                else
                                {
                                    var newDNS = new ValidDNS();
                                    newDNS.IpAddress = GlobalData.TerciaryDnsIp;

                                    optList.Add(new[] {newDNS});
                                    tempSplitedServers.Add(GlobalData.TerciaryDnsIp);
                                }
                                sValidDnsListRemediate = string.Join(",", tempSplitedServers.ToArray());
                            }

                            break;
                    }

                    validDns.IsValid = false;
                }

                if (validDns.IsValid) //IsValid means is existing/configured on NIC
                {
                    if (validDns.IpAddress.Equals(LoopbackIPv4Address, StringComparison.InvariantCultureIgnoreCase))
                    {
                        validDns.IsValid = false;
                    }
                    else
                    {
                        switch (i)
                        {
                            case 0: //pri
                                if (!GlobalData.PrimaryDnsAuto)
                                {
                                    if (validDns.IpAddress.Equals(GlobalData.PrimaryDnsIp))
                                    {
                                        isOptimalDns = true;
                                    }
                                }
                                break;

                            case 1: //alt
                                if (!GlobalData.AlternateDnsAuto)
                                {
                                    if (validDns.IpAddress.Equals(GlobalData.AlternateDnsIp))
                                    {
                                        isOptimalDns = true;
                                    }
                                    else //manual ip is different then nice sugestion!
                                    {
                                        isOptimalDns = false;
                                    }

                                    var tempSplitedServers = new List<string>();
                                    var tempSplitedServersArray = sValidDnsListRemediate.Split(',');
                                    tempSplitedServers.AddRange(tempSplitedServersArray);

                                    if (optList.Count >= i + 1)
                                    {
                                        if (!tempSplitedServers[i].Equals(GlobalData.AlternateDnsIp))
                                        {
                                            tempSplitedServers[i] = GlobalData.AlternateDnsIp;

                                            var newDNS = new ValidDNS();
                                            newDNS.IpAddress = GlobalData.AlternateDnsIp;

                                            optList[i] = new[] {newDNS};
                                        }
                                    }
                                    else
                                    {
                                        var newDNS = new ValidDNS();
                                        newDNS.IpAddress = GlobalData.AlternateDnsIp;

                                        optList.Add(new[] {newDNS});
                                        tempSplitedServers.Add(GlobalData.AlternateDnsIp);
                                    }
                                    sValidDnsListRemediate = string.Join(",", tempSplitedServers.ToArray());
                                }

                                break;
                            case 2: //pri
                                if (!GlobalData.TerciaryDnsAuto)
                                {
                                    if (validDns.IpAddress.Equals(GlobalData.TerciaryDnsIp))
                                    {
                                        isOptimalDns = true;
                                    }
                                    else
                                    {
                                        isOptimalDns = false;
                                    }

                                    var tempSplitedServers = new List<string>();
                                    var tempSplitedServersArray = sValidDnsListRemediate.Split(',');
                                    tempSplitedServers.AddRange(tempSplitedServersArray);


                                    if (tempSplitedServers.Count >= i + 1)
                                    {
                                        if (!tempSplitedServers[i].Equals(GlobalData.TerciaryDnsIp))
                                        {
                                            tempSplitedServers[i] = GlobalData.TerciaryDnsIp;

                                            var newDNS = new ValidDNS();
                                            newDNS.IpAddress = GlobalData.TerciaryDnsIp;

                                            optList[i] = new[] {newDNS};
                                        }
                                    }
                                    else if (tempSplitedServers.Count == 1)
                                    {
                                        tempSplitedServers.Add(GlobalData.TerciaryDnsIp);

                                        Console.ForegroundColor = ConsoleColor.DarkYellow;
                                        Console.WriteLine();
                                        Console.WriteLine("Information:\tTertiary DNS will be set as alternate DNS");

                                        Logger.Warn("Tertiary DNS will be set as alternate DNS");

                                        var newDNS = new ValidDNS
                                                     {
                                                         IpAddress = GlobalData.TerciaryDnsIp
                                                     };

                                        optList.Add(new[] {newDNS});
                                    }
                                    else
                                    {
                                        var newDNS = new ValidDNS();
                                        newDNS.IpAddress = GlobalData.TerciaryDnsIp;

                                        optList.Add(new[] {newDNS});
                                        tempSplitedServers.Add(GlobalData.TerciaryDnsIp);
                                    }

                                    sValidDnsListRemediate = string.Join(",", tempSplitedServers.ToArray());
                                }
                                break;
                        }
                    }
                }

                dnsConfiguredOnNIc.Add(validDns);
            }

            for (var i = 0; i <= 2; i++)
            {
                var validDns = dnsConfiguredOnNIc[i];

                var dnsPositionName = "primary";
                switch (i)
                {
                    case 1:
                        dnsPositionName = "alternate";
                        break;
                    case 2:
                        dnsPositionName = "tertiary";
                        break;
                }

                if (optList.Count < i + 1)
                {
                    Console.ForegroundColor = ConsoleColor.Green;
                    Console.WriteLine("Information:\tOptimal {0} DNS Server cannot be provided.", dnsPositionName);

                    Logger.Warn("Optimal {0} DNS Server cannot be determined.", dnsPositionName);
                }
                else
                {
                    //if DNS server is not valid it also should be detected as not optimal, because optlist is filtered only with valid dns servers from database

                    isOptimalDns = IsDnsOptimalNoLoadBalancing(validDns.IpAddress, i, optList);

                    if (isOptimalDns)
                    {
                        Console.ResetColor();
                        Console.ForegroundColor = ConsoleColor.Green;
                        Console.WriteLine("Information:\t{0} DNS Server with IPv4 address [{1}] is configured optimally.", UppercaseFirst(dnsPositionName), validDns.IpAddress);

                        Logger.Warn("{0} DNS Server with IPv4 address [{1}] is configured optimally.", UppercaseFirst(dnsPositionName), validDns.IpAddress);
                    }
                    else
                    {
                        var displayTitle = string.Format("Warning:\t{1} DNS Server with IPv4 address [{0}] is not configured optimally.", validDns.IpAddress, UppercaseFirst(dnsPositionName));


                        Console.ForegroundColor = ConsoleColor.Yellow;
                        var temp = Program.SplitStringByLineLentgh(100, displayTitle);

                        foreach (var tempStr in temp)
                        {
                            Console.WriteLine(tempStr);
                        }

                        Logger.Warn(displayTitle);

                        suggestedNewDnsList = true;
                    }
                }
            }

            if (suggestedNewDnsList)
            {
                Console.ForegroundColor = ConsoleColor.Green;
                Console.WriteLine("Information:\tSuggested optimal DNS Servers:");

                Logger.Info("Suggested optimal DNS Servers:");

                var tempSplitedServers = new List<string>();
                var tempSplitedServersArray = sValidDnsListRemediate.Split(',');
                tempSplitedServers.AddRange(tempSplitedServersArray);


                foreach (var serverIp in tempSplitedServers)
                {
                    var next = false;

                    foreach (ValidDNS[] validDnsArray in optList)
                    {
                        if (next) break;

                        foreach (var validDns in validDnsArray)
                        {
                            if (validDns.IpAddress.Equals(serverIp, StringComparison.CurrentCulture))
                            {
                                Console.WriteLine("Information:\tDNS Server {0} in domain {3} with {1} found in site {2}", validDns.ServerName, validDns.IpAddress, validDns.SiteName, validDns.DomainName);

                                Logger.Info("DNS Server {0} in domain {3} with {1} found in site {2}", validDns.ServerName, validDns.IpAddress, validDns.SiteName, validDns.DomainName);

                                next = true;
                                break;
                            }
                        }
                    }
                }

                if (!GlobalData.ReadOnly)
                {
                    if (tempSplitedServers.Count > 3)
                    {
                        sValidDnsListRemediate = string.Join(",", tempSplitedServers.GetRange(0, 3).ToArray());
                    }


                    var remediationSuccess = NotifyAndRemediateDNS(nic, sValidDnsListRemediate);

                    if (remediationSuccess)
                    {
                        Console.ResetColor();
                        Console.ForegroundColor = ConsoleColor.Green;
                        Console.WriteLine("Information:\tRemediation performed successfully");

                        Logger.Info("Remediation performed successfully");
                    }
                    else
                    {
                        Console.ForegroundColor = ConsoleColor.Red;
                        Console.WriteLine();
                        Console.WriteLine("Warning:\tRemediation encountered an error");

                        Logger.Error("Remediation encountered an error");
                    }

                    Logger.Info("Network configuration after remediation:");

                    //nic = Program.GetIntefaceByName(GlobalData.NetworkConnectionName);
                    LogNicConfiguration(GlobalData.NetworkInterface);
                }
                else
                {
                    Console.ForegroundColor = ConsoleColor.DarkYellow;
                    Console.WriteLine();
                    Console.WriteLine("Information:\tRun Application again with UPDATE parameter to perform remediation");

                    Logger.Info("Run Application again with UPDATE parameter to perform remediation");
                }
            }
        }

        private static void LogNicConfiguration(NetworkInterface nic)
        {
            var properties = nic.GetIPProperties();

            //LINQ
            //var ipv4DNSAddresses = Array.FindAll(
            //    properties.DnsAddresses.ToArray(),
            //    a => a.AddressFamily == AddressFamily.InterNetwork);

            List<IPAddress> ipv4DNSAddresses = new List<IPAddress>();

            foreach (var dnsAddress in properties.DnsAddresses)
            {
                if (dnsAddress.AddressFamily == AddressFamily.InterNetwork)
                {
                    ipv4DNSAddresses.Add(dnsAddress);
                }
            }

            //LINQ
            //var nicIp = properties.UnicastAddresses.Where(adr => !IPAddress.IsLoopback(adr.Address))
            //    .FirstOrDefault(adr => adr.Address.AddressFamily == AddressFamily.InterNetwork);
            IPAddress nicIpAddress = null;
            IPAddress sub = null;

            foreach (var adr in properties.UnicastAddresses)
            {
                if (!IPAddress.IsLoopback(adr.Address) && (adr.Address.AddressFamily == AddressFamily.InterNetwork))
                {
                    nicIpAddress = adr.Address;
                    sub = adr.IPv4Mask;
                    break;
                }
            }


            var gatewayAddresses = properties.GatewayAddresses;
            string gtw = string.Empty;
            if ((gatewayAddresses.Count > 0) && (gatewayAddresses[0] != null))
            {
                gtw = gatewayAddresses[0].Address.ToString();
            }

            Logger.Info("Hostname:\t\t\t[{0}]", GlobalData.MachineFqName);
            Logger.Info("Ethernet Adapter:\t[{0}]", nic.Name);
            Logger.Info("IPv4 Address:\t\t[{0}]", nicIpAddress);
            Logger.Info("Subnet Mask:\t\t[{0}]", sub);
            Logger.Info("Default Gateway:\t[{0}]", gtw);

            for (var i = 0; i < ipv4DNSAddresses.Count; i++)
            {
                var dnsIp = ipv4DNSAddresses[i];
                Logger.Info("DNS Server\t\t\t[{1}]: [{0}]", dnsIp, i + 1);
            }

            //LINQ
            //winsServerListConfiguredOnNic = GetWinsOnNicFromRegistry(nic.Id).Where(w => !string.IsNullOrEmpty(w)).ToList();

            var winsServerListConfiguredOnNic = GetWinsOnNicFromRegistry(nic.Id);

            for (var i = 0; i < winsServerListConfiguredOnNic.Count; i++)
            {
                string winsIp = winsServerListConfiguredOnNic[i];
                if (string.IsNullOrEmpty(winsIp)) continue; //probably in some tests there were empty wins in list?!

                Logger.Info("WINS Server\t\t\t[{1}]: [{0}]", winsIp, i + 1);
            }
        }

        private static List<string> GetRegistryValue(string registryPath, string key)
        {
            var resultList = new List<string>();

            try
            {
                // int lastindex = registryPath.LastIndexOf("\\", StringComparison.Ordinal);
                // string key = registryPath.Substring(0, lastindex);
                var val = Registry.GetValue(registryPath, key, null);

                if (val == null) return resultList;

                //LINQ
                //resultList = ((string[]) val).ToList();

                resultList.AddRange((string[]) val);
            }
            catch (Exception)
            {
                // throw;
            }
            return resultList;
        }

        public static List<string> GetWinsOnNicFromRegistry(string nicGuid)
        {
            var winsList = new List<string>();
            try
            {
                var key = "NameServerList";
                string regpath = string.Format(@"HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\NetBT\Parameters\Interfaces\Tcpip_{0}", nicGuid);

                var result = GetRegistryValue(regpath, key);
                if (result != null)
                {
                    winsList = result;
                }
            }
            catch (Exception ex)
            {
                Logger.LogException(LogLevel.Error, "GetWinsOnNicFromRegistry", ex);
            }

            return winsList;
        }

        /// <summary>
        ///     Checks validity of server, but only portQuery53 condition is used.
        /// </summary>
        /// <param name="server"> object from database types. </param>
        /// <returns> true if server is valid. false if server cannot be validated </returns>
        public static bool CheckValidityOfValidDNSReduced(ref ValidDNS server)
        {
            var result = false;

            try
            {
                var pc = new PortCheck();
                pc.IpType = "TCP";
                pc.PortNumber = 53;
                pc.Value = PortCheckTcp(server.IpAddress, pc);
                var port53 = pc.Value;

                foreach (var portCheck in server.PortCheck)
                {
                    if (portCheck.PortNumber == pc.PortNumber &&
                        portCheck.IpType.Equals(pc.IpType, StringComparison.InvariantCultureIgnoreCase))
                    {
                        portCheck.Value = port53;
                    }
                }

                if (port53)
                {                    
                    server.IsValid = true;
                    result = true;
                }
                else
                {                    
                    server.Note += "PortCheck53 problem. ";                    
                    server.IsValid = false;
                }
            }
            catch (Exception ex)
            {
                Logger.LogException(LogLevel.Error, "CheckValidityOfValidDNSReduced", ex);

                server.IsValid = false;
                return false;
            }

            return result;
        }

        /// <summary>
        ///     Port query specified port.
        /// </summary>
        /// <param name="iPAddress"> The machine address. </param>
        /// <param name="portCheck"> portCheck object, holds ipType, PortNumber </param>
        /// <returns> True if port connection is established </returns>
        public static bool PortCheckTcp(string iPAddress, PortCheck portCheck)
        {
            try
            {
                //tcp port parameters
                var timeOut = 200;
                var numberOfChecksInTry = 10;
                var numberOfTries = 2;
                return PortQueryTcpConnect(iPAddress, portCheck.PortNumber, timeOut, numberOfChecksInTry, numberOfTries);
            }
            catch
            {
                return false; // An exception occured, thus the port is probably closed
            }
        }

        /// <summary>
        ///     Port query specified port.
        /// </summary>
        /// <param name="iPAddress"> The machine address. </param>
        /// <param name="port"> port number. </param>
        /// <param name="timeOut"> Time in ms. between each check </param>
        /// <param name="numberOfChecksInTry"> The number of checks in one try to establish the connection </param>
        /// <param name="numberOfTries"> The number of tries to establish the connection </param>
        /// <returns> True if port connection is established </returns>
        public static bool PortQueryTcpConnect(string iPAddress, int port, int timeOut, int numberOfChecksInTry, int numberOfTries)
        {
            var connectSuccess = false;
            try
            {
                // Try to connect 

                for (var j = 1; j <= numberOfTries; j++)
                {
                    var tcpScan = new TcpClient();
                    tcpScan.BeginConnect(iPAddress, port, null, null);
                    for (var i = 1; i <= numberOfChecksInTry; i++)
                    {
                        Thread.Sleep(timeOut); //wait 1 sec
                        if (tcpScan.Connected)
                        {
                            connectSuccess = true;
                            tcpScan.Close();
                            break;
                        }

                        if (i == numberOfChecksInTry)
                        {
                            tcpScan.Close();
                        }
                    }
                    if (connectSuccess)
                    {
                        break;
                    }
                    if (j == numberOfTries)
                    {
                        //Console.WriteLine("****************************************");
                        //Console.WriteLine(String.Format("PortQuery.Connect reached last try. Params used: timeOut={0}, numberOfChecksInTry={1}, numberOfTries={2}", timeOut, numberOfChecksInTry, numberOfTries));
                        //Console.WriteLine("****************************************");
                    }
                    //return connectSuccess;
                    // If there\'s no exception, we can say the port is open 
                }
            }
            catch
            {
                return false; // An exception occured, thus the port is probably closed
            }

            return connectSuccess;
        }


        private static bool NotifyAndRemediateDNS(NetworkInterface nic, string dnsIpPAddress)
        {
            return DnsRemediate.SetDns(nic.Id, dnsIpPAddress);
        }

        public static List<ValidDNS[]> OptimalDnsListNoLoadBalancing(List<ValidDNS> listValidDns, out string sOptimalDnsListNoLoadBalancingRemediate, string nicIpAddress)
        {
            var optimalDnsList = new List<ValidDNS[]>();
            sOptimalDnsListNoLoadBalancingRemediate = string.Empty;

            if (listValidDns.Count > 0)
            {
                //LINQ
                //var allServersGrouped = from ss in listValidDns
                //   //maybe remove orderby, it is allready ordered like this when reading from databae
                //    orderby ss.Order, ss.SiteLevel, ss.ServerName
                //    group ss by ss.Order
                //    into ss
                //    select ss;


                //NOTE: if server is INVALID than  ratingmark = 0, set in methodes before this
                //   List<List<ValidDNS>> allServersGroupedByRatingMark = new List<List<ValidDNS>>();
                listValidDns.Sort((x, y) => x.RatingMark.CompareTo(y.RatingMark));
                listValidDns.Reverse();

                //int previousOrder = -1;
                //List<ValidDNS> newGroup = new List<ValidDNS>();
                //foreach (var validDNS in listValidDns)
                //{

                //    int currentOrder = validDNS.Order;
                //    if (previousOrder == -1)  //first element in the list
                //    {
                //        previousOrder = currentOrder;
                //        newGroup.Add(validDNS);
                //    }
                //    else //rest of the list
                //    {

                //    if (currentOrder == previousOrder )  //still fill into same group
                //    {
                //        newGroup.Add(validDNS);
                //    }

                //    else
                //    {
                //        allServersGroupedByRatingMark.Add(newGroup);
                //        previousOrder = currentOrder;
                //            newGroup = new List<ValidDNS>();
                //        newGroup.Add(validDNS);
                //    }

                //    }
                //}
                ////after looping all dns servers
                //if (newGroup.Count > 0)
                //{
                //    allServersGroupedByRatingMark.Add(newGroup);
                //}


                List<List<ValidDNS>> groupByRatingMarkList = new List<List<ValidDNS>>();
                List<ValidDNS> newValidDnsList = new List<ValidDNS>();
                int previousRatingMark = 100000;
                foreach (var validDNS in listValidDns)
                {
                    ValidDNS newValidDns = validDNS;
                    int currentRatingMark = newValidDns.RatingMark;
                    if (previousRatingMark == 100000) //first element in the list
                    {
                        previousRatingMark = currentRatingMark;
                        newValidDnsList.Add(newValidDns);
                    }
                    else //rest of the list
                    {
                        if (currentRatingMark == previousRatingMark) //still fill into same group
                        {
                            newValidDnsList.Add(newValidDns);
                        }

                        else
                        {
                            groupByRatingMarkList.Add(newValidDnsList);
                            previousRatingMark = currentRatingMark;
                            newValidDnsList = new List<ValidDNS>();
                            newValidDnsList.Add(newValidDns);
                        }
                    }
                }

                //after looping all dns servers
                if (newValidDnsList.Count > 0)
                {
                    groupByRatingMarkList.Add(newValidDnsList);
                }


                var serversTotal = listValidDns.Count;

                sOptimalDnsListNoLoadBalancingRemediate = string.Empty;

                optimalDnsList = new List<ValidDNS[]>(serversTotal);

                //once iterate list for optimality check

                var kk = 0;
                foreach (var group in groupByRatingMarkList)
                {
                    foreach (var adSiteServer in group)
                    {
                        if (adSiteServer.IpAddress.Equals(LoopbackIPv4Address, StringComparison.InvariantCultureIgnoreCase)) continue;

                        optimalDnsList.Add(group.ToArray());

                        if (kk == 0)
                        {
                            sOptimalDnsListNoLoadBalancingRemediate += adSiteServer.IpAddress;
                        }
                        else
                        {
                            if (kk < GlobalData.MaximumDNSServers) // // if (kk <= 3)

                            {
                                sOptimalDnsListNoLoadBalancingRemediate += "," + adSiteServer.IpAddress;
                            }
                        }

                        kk = +1;
                    }
                }
            }

            return optimalDnsList;
        }

        public static bool IsDnsOptimalNoLoadBalancing(string dnsIpAddress, int order, List<ValidDNS[]> optimalDnsListNoLoadBalancing)
        {
            try
            {
                if (string.IsNullOrEmpty(dnsIpAddress))
                {
                    return false;
                }

                foreach (var server in optimalDnsListNoLoadBalancing[order])
                {
                    if (server.IpAddress.Equals(dnsIpAddress, StringComparison.InvariantCultureIgnoreCase))
                    {
                        return true;
                    }
                }
            }
            catch (Exception ex)
            {
                Logger.LogException(LogLevel.Error, "IsDnsOptimalNoLoadBalancing", ex);
            }

            return false;
        }

        private static string UppercaseFirst(string s)
        {
            // Check for empty string.
            if (string.IsNullOrEmpty(s))
            {
                return string.Empty;
            }
            // Return char and concat substring.
            return char.ToUpper(s[0]) + s.Substring(1);
        }
    }
}