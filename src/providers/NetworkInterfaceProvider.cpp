
 class NetworkInterfaceProvider : BaseProvider
    {
        private:
         readonly Logger _Logger;

        public:
         NetworkInterfaceProvider(Options options) : base(options)
        {
            _Logger = LogManager.GetCurrentClassLogger();
        }

         NetworkInterface GetDefaultInteface()
        {
            try
            {
                var allNetworkInterfaces = NetworkInterface.GetAllNetworkInterfaces().Where(x => x.OperationalStatus == OperationalStatus.Up
                                                                                                 && (x.NetworkInterfaceType == NetworkInterfaceType.Ethernet
                                                                                                     || x.NetworkInterfaceType == NetworkInterfaceType.GigabitEthernet
                                                                                                     || x.NetworkInterfaceType == NetworkInterfaceType.Ethernet3Megabit
                                                                                                     || x.NetworkInterfaceType == NetworkInterfaceType.FastEthernetFx
                                                                                                     || x.NetworkInterfaceType == NetworkInterfaceType.FastEthernetT)).ToList();

                if (string.IsNullOrEmpty(Options.NetworkInterfaceName))
                {
                    _Logger.Info("No network adapter name was specified. The tool will asume that computer has only one NIC and will try to use that");

                    if (allNetworkInterfaces.Count == 1)
                    {
                        return allNetworkInterfaces[0];
                    }

                    return null;
                }

                var nic = allNetworkInterfaces.FirstOrDefault(x => x.Name.Equals(Options.NetworkInterfaceName, StringComparison.InvariantCultureIgnoreCase)) ??
                          allNetworkInterfaces.FirstOrDefault(x => x.Description.Equals(Options.NetworkInterfaceName, StringComparison.InvariantCultureIgnoreCase));

                return nic;
            }
            catch (Exception ex)
            {
                Console.ResetColor();
                Console.WriteLine("Log information is stored in the file: {0}", Options.LogFilePath);

                _Logger.LogException(LogLevel.Error, "General error", ex);
            }

            return null;
        }

         bool SetDNSServers(string networkInterfaceId, string dnsServers)
        {
            if (string.IsNullOrEmpty(networkInterfaceId)) throw new ArgumentNullException(nameof(networkInterfaceId));
            if (string.IsNullOrEmpty(dnsServers)) throw new ArgumentNullException(nameof(dnsServers));

            ManagementClass objMc = new ManagementClass("Win32_NetworkAdapterConfiguration");
            ManagementObjectCollection objMoc = objMc.GetInstances();

            foreach (ManagementObject objMo in objMoc)
            {
                if (objMo["SettingID"].Equals(networkInterfaceId))
                {
                    try
                    {
                        ManagementBaseObject newDNS = objMo.GetMethodParameters("SetDNSServerSearchOrder");
                        newDNS["DNSServerSearchOrder"] = dnsServers.Split(',');

                        ManagementBaseObject setDNS = objMo.InvokeMethod("SetDNSServerSearchOrder", newDNS, null);

                        return true;
                    }
                    catch (Exception ex)
                    {
                        _Logger.LogException(LogLevel.Error, "SetDNSServers", ex);
                    }
                }
            }

            return false;
        }

         bool SetWINSServers(string networkInterfaceId, string wins1, string wins2)
        {
            ManagementClass objMc = new ManagementClass("Win32_NetworkAdapterConfiguration");
            ManagementObjectCollection objMoc = objMc.GetInstances();

            foreach (ManagementObject objMo in objMoc)
            {
                if (objMo["SettingID"].Equals(networkInterfaceId))
                {
                    try
                    {                        
                        ManagementBaseObject wins = objMo.GetMethodParameters("SetWINSServer");
                        wins.SetPropertyValue("WINSPrimaryServer", wins1);
                        wins.SetPropertyValue("WINSSecondaryServer", wins2);

                        ManagementBaseObject setWINS = objMo.InvokeMethod("SetWINSServer", wins, null);

                        return true;
                    }
                    catch (Exception ex)
                    {
                        _Logger.LogException(LogLevel.Error, "SetDNSServers", ex);
                    }
                }                   
            }

            return false;
        }

         void WriteWINSServers(string networkInterfaceId)
        {
            ManagementClass objMc = new ManagementClass("Win32_NetworkAdapterConfiguration");
            ManagementObjectCollection objMoc = objMc.GetInstances();

            foreach (ManagementObject objMo in objMoc)
            {
                if (objMo["SettingID"].Equals(networkInterfaceId))
                {
                    try
                    {
                        if (objMo["WINSPrimaryServer"] != null)
                        {
                            _Logger.Info("WINS Server\t[{1}]:[{0}]", objMo["WINSPrimaryServer"], 1);
                        }

                        if (objMo["WINSSecondaryServer"] != null)
                        {
                            _Logger.Info("WINS Server\t[{1}]:[{0}]", objMo["WINSSecondaryServer"], 2);
                        }                                               
                    }
                    catch (Exception ex)
                    {
                        _Logger.LogException(LogLevel.Error, "SetDNSServers", ex);
                    }

                    return;
                }
            }
        }

         void WriteNetworkInformation(NetworkInterface nic)
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

            _Logger.Info("Hostname:\t\t[{0}]", Environment.MachineName);
            _Logger.Info("Ethernet Adapter:\t[{0}]", nic.Name);
            _Logger.Info("IPv4 Address:\t[{0}]", nicIpAddress);
            _Logger.Info("Subnet Mask:\t[{0}]", sub);
            _Logger.Info("Default Gateway:\t[{0}]", gtw);

            for (var i = 0; i < ipv4DNSAddresses.Count; i++)
            {
                var dnsIp = ipv4DNSAddresses[i];
                _Logger.Info("DNS Server\t[{1}]:[{0}]", dnsIp, i + 1);
            }

            if (Options.ExecutionOption == ExecutionOption.WINS)
            {
                WriteWINSServers(nic.Id);
            }

            //LINQ
            //winsServerListConfiguredOnNic = GetWinsOnNicFromRegistry(nic.Id).Where(w => !string.IsNullOrEmpty(w)).ToList();

            //var winsServerListConfiguredOnNic = GetWinsOnNicFromRegistry(nic.Id);

            //for (var i = 0; i < winsServerListConfiguredOnNic.Count; i++)
            //{
            //    string winsIp = winsServerListConfiguredOnNic[i];
            //    if (string.IsNullOrEmpty(winsIp)) continue; //probably in some tests there were empty wins in list?!

            //    _Logger.Info("WINS Server\t\t\t[{1}]: [{0}]", winsIp, i + 1);
            //}
        }
    };
