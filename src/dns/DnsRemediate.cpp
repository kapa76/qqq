
      class DnsRemediate
    {
        static Logger Logger = LogManager.GetCurrentClassLogger();


        public:

         static bool SetDns(string nic, string dns)
        {
            ManagementClass objMc = new ManagementClass("Win32_NetworkAdapterConfiguration");
            ManagementObjectCollection objMoc = objMc.GetInstances();

            foreach (ManagementObject objMo in objMoc)
            {
                if (objMo["SettingID"].Equals(nic))
                {
                    try
                    {
                        ManagementBaseObject newDNS = objMo.GetMethodParameters("SetDNSServerSearchOrder");
                        newDNS["DNSServerSearchOrder"] = dns.Split(',');

                        ManagementBaseObject setDNS = objMo.InvokeMethod("SetDNSServerSearchOrder", newDNS, null);

                        return true;
                    }
                    catch (Exception ex)
                    {
                        Logger.LogException(LogLevel.Error, "General error", ex);
                    }
                }
            }

            return false;
        }
    };
