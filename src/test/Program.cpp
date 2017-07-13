using System;
using System.Collections.Generic;
using System.DirectoryServices.ActiveDirectory;
using System.DirectoryServices.Protocols;
using System.Linq;
using System.Net;
using System.Net.NetworkInformation;
using System.Net.Sockets;
using System.Text;
using System.Threading.Tasks;

namespace Test
{
    class Program
    {
        static void Main(string[] args)
        {
            try
            {
                StringBuilder sb = new StringBuilder();
                sb.Append(",123.123.123.123,111.1.12.111");

                Console.WriteLine(sb.ToString().Remove(0, 1));

                //SmartWriteLine("Information", "OK this is very long text that has to be cut smartly; lets see how that will be done with this code and if there will be some problems. It usese recursion and the lenght may throw argument out of bounds.");

                //SmartWriteLine("Information", "OK short");

                //SmartWriteLine("Information", "OK this is very long text that has to be cut smartly; lets see how that will be done with this code and if there will be some problems. It usese recursion and the lenght may throw argument out of bounds. But what about the 3 line");
                //var nic = GetDefaultInteface();

                //var properties = nic.GetIPProperties();

                //List<IPAddress> ipv4DNSAddresses = new List<IPAddress>();

                //foreach (var dnsAddress in properties.DnsAddresses)
                //{
                //    if (dnsAddress.AddressFamily == AddressFamily.InterNetwork)
                //    {
                //        ipv4DNSAddresses.Add(dnsAddress);
                //    }
                //}

                //string sValidDnsListRemediate;
                //bool isOptimalDns;


                //LINQ
                //var nicIpAddress = properties.UnicastAddresses.Where(adr => !IPAddress.IsLoopback(adr.Address))
                //        .FirstOrDefault(adr => adr.Address.AddressFamily == AddressFamily.InterNetwork).Address.ToString();

                //string nicIpAddress = string.Empty;
                //foreach (var adr in properties.UnicastAddresses)
                //{
                //    if (!IPAddress.IsLoopback(adr.Address) && (adr.Address.AddressFamily == AddressFamily.InterNetwork))
                //    {
                //        nicIpAddress = adr.Address.ToString();
                //        break;
                //    }
                //}

                //var d = Domain.GetCurrentDomain();
                //DirectoryContext ct = new DirectoryContext(DirectoryContextType.Forest, "SIGN.LOCAL");

                //using (var connectedSite = ActiveDirectorySite.FindByName(ct, "US-NYC"))
                //{
                //    Console.WriteLine(connectedSite.Name);
                //}

                //TestSiteLinks();
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex);
            }

            Console.ReadKey();
        }

        public static void SmartWriteLine(string label, string msg)
        {
            string tmpMsg = msg;

            while (!string.IsNullOrEmpty(tmpMsg))
            {
                tmpMsg = tmpMsg.TrimStart();
                Console.Write("{0}\t", label);

                string s = SmartTrim(tmpMsg, Console.BufferWidth - Console.CursorLeft);

                Console.WriteLine(s);
                tmpMsg = tmpMsg.Remove(0, s.Length);
            }
        }

        public static string SmartTrim(string text, int length)
        {
            if (string.IsNullOrEmpty(text))
            {
                throw new ArgumentNullException(nameof(text));
            }

            if (text.Length <= length)
            {
                return text;
            }

            int lastSpaceBeforeMax = text.LastIndexOf(' ', length);

            if (lastSpaceBeforeMax == -1)
            {
                return text;
            }

            return text.Substring(0, lastSpaceBeforeMax);
        }

        private static void TestSiteLinks()
        {
            using (LdapConnection ldap = new LdapConnection("SIGN.LOCAL"))
            {

                List<string> attributes = new List<string>
                                              {
                                                  "name",
                                                  "cost",
                                                  "siteList",
                                                  "distinguishedName"
                                              };
                
                string filter = string.Format("(&(objectClass=siteLink)(siteList={0}))", "CN=US-NYC,CN=Sites,CN=Configuration,DC=SIGN,DC=LOCAL");
                SearchRequest searchRequest = new SearchRequest("CN=Sites,CN=Configuration,DC=SIGN,DC=LOCAL", filter, SearchScope.Subtree, attributes.ToArray());

                SearchResponse searchResponse = ldap.SendRequest(searchRequest) as SearchResponse;

                //zemi gi site linkovi
                //podredigi linkovite po cost
                //

                if (searchResponse != null)
                {
                    
                    foreach (SearchResultEntry entry in searchResponse.Entries)
                    {
                        Console.WriteLine(entry.Attributes["name"][0].ToString());
                        Console.WriteLine(entry.Attributes["distinguishedName"][0].ToString());
                        Console.WriteLine(entry.Attributes["cost"][0].ToString());                        
                    }
                }
                else
                {
                    Console.WriteLine("No Active Directory site links found");
                }
            }

        }

        public static NetworkInterface GetDefaultInteface()
        {
            try
            {
                var allNetworkInterfaces = NetworkInterface.GetAllNetworkInterfaces().Where(x => x.OperationalStatus == OperationalStatus.Up
                                                                                                 && (x.NetworkInterfaceType == NetworkInterfaceType.Ethernet
                                                                                                     || x.NetworkInterfaceType == NetworkInterfaceType.GigabitEthernet
                                                                                                     || x.NetworkInterfaceType == NetworkInterfaceType.Ethernet3Megabit
                                                                                                     || x.NetworkInterfaceType == NetworkInterfaceType.FastEthernetFx
                                                                                                     || x.NetworkInterfaceType == NetworkInterfaceType.FastEthernetT)).ToList();

                return allNetworkInterfaces[0];
            }
            catch (Exception ex)
            {
                Console.ResetColor();
               // Console.WriteLine("Log information is stored in the file: {0}", Options.LogFilePath);


            }

            return null;
        }
    }
}
