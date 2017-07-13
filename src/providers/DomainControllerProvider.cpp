using System;
using System.Collections.Generic;
using System.DirectoryServices.ActiveDirectory;
using System.DirectoryServices.Protocols;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using NLog;
using Synergix.ADCE.Lite.Commands;
using Synergix.ADCE.Lite.Objects;
using ActiveDirectorySite = System.DirectoryServices.ActiveDirectory.ActiveDirectorySite;
using ActiveDirectorySiteLink = System.DirectoryServices.ActiveDirectory.ActiveDirectorySiteLink;
using DomainController = Synergix.ADCE.Lite.Objects.DomainController;

namespace Synergix.ADCE.Lite.Providers
{
    public class DomainControllerProvider : BaseProvider
    {
        private readonly CommandContext _Context;
        private readonly Logger _Logger;
        private readonly List<string> _CheckedSites;

        private readonly List<DomainController> _AllDiscoveredDomainControllers;

        public DomainControllerProvider(Options options, CommandContext context) : base(options)
        {
            if (context == null) throw new ArgumentNullException(nameof(context));

            _Context = context;
            _Logger = LogManager.GetCurrentClassLogger();
            _AllDiscoveredDomainControllers = new List<DomainController>();
            _CheckedSites = new List<string>();
        }

        public List<DomainController> AllDiscoveredDomainControllers
        {
            get { return _AllDiscoveredDomainControllers; }
        }

        public IList<DomainController> GetDomainControllersForDomainByClosesedSite(int maxServersToReturn)
        {
            int maxServers = maxServersToReturn;
            using (var currentComputerSite = ActiveDirectorySite.GetComputerSite())
            {
                using (var currentDomain = Domain.GetComputerDomain())
                {
                    var domainControllers = GetDomainControllersInSite(currentDomain, currentComputerSite.Name, 0);

                    _AllDiscoveredDomainControllers.AddRange(domainControllers);

                    Shuffle(ref domainControllers);

                    var siteDomains = domainControllers.Take(Options.CurrentSiteNumberOfServers).ToList();

                    for (int i = 0; i < siteDomains.Count; i++)
                    {
                        DomainController controller = siteDomains[i];
                        controller.Order = (i + 1)*-1;
                    }

                    if (siteDomains.Count >= maxServers) return siteDomains;

                    var dcs = GetConnectedSiteDomainControllers(currentDomain, currentComputerSite, 0, maxServers - siteDomains.Count, 1);

                    foreach (var dc in dcs)
                    {
                        if(siteDomains.Any(x => x.IPAddress.Equals(dc.IPAddress))) continue;

                        siteDomains.Add(dc);
                    }                    

                    return siteDomains;
                }
            }            
        }

        private List<DomainController> GetConnectedSiteDomainControllers(Domain domain, ActiveDirectorySite site, int cost, int maxServersToReturn, int level)
        {
            if (level > 3) return new List<DomainController>();

            _CheckedSites.Add(site.Name);

            List<Objects.ActiveDirectorySite> connectedSites = new List<Objects.ActiveDirectorySite>();

            bool hasOverridenSites = false;

            List<ActiveDirectorySiteLink> siteLinks = new List<ActiveDirectorySiteLink>();

            foreach (ActiveDirectorySiteLink link in site.SiteLinks)
            {
                siteLinks.Add(link);
            }

            foreach (ActiveDirectorySiteLink link in siteLinks.OrderBy(x => x.Cost))
            {
                foreach (ActiveDirectorySite site1 in link.Sites)
                {
                    if (site1.Name.Equals(site.Name, StringComparison.InvariantCultureIgnoreCase)) continue;

                    if (_CheckedSites.Any(x => site1.Name.Equals(x))) continue;

                    var single = connectedSites.SingleOrDefault(x => x.Name.Equals(site1.Name, StringComparison.InvariantCultureIgnoreCase));

                    if (single == null)
                    {
                        using (var siteDe = site1.GetDirectoryEntry())
                        {
                            try
                            {
                                var siteDnObj = siteDe.Properties["distinguishedName"];

                                if (siteDnObj?.Value != null)
                                {
                                    string siteDn = siteDnObj.Value.ToString();

                                    var overridenSitesList = GetOverridenSites(domain.Name, siteDn);

                                    if (overridenSitesList != null)
                                    {
                                        var overridenSites = overridenSitesList.Where(x => x.Type == SiteOverrideType.DNS).ToList();

                                        if (overridenSites.Count > 0)
                                        {
                                            hasOverridenSites = true;

                                            foreach (var overridenSite in overridenSites)
                                            {
                                                Objects.ActiveDirectorySite overridenSiteName = GetSiteByDistinguishedName(domain.Name, overridenSite.DistinguishedName);

                                                if (overridenSiteName != null)
                                                {
                                                    overridenSiteName.Cost = cost + overridenSite.Order;
                                                    connectedSites.Add(overridenSiteName);
                                                }
                                            }
                                        }
                                        else
                                        {
                                            connectedSites.Add(new Objects.ActiveDirectorySite
                                                               {
                                                                   Name = site1.Name,
                                                                   DistinguishedName = siteDn,
                                                                   Cost = cost + link.Cost
                                                               });
                                        }
                                    }
                                    else
                                    {
                                        connectedSites.Add(new Objects.ActiveDirectorySite
                                                           {
                                                               Name = site1.Name,
                                                               DistinguishedName = siteDn,
                                                               Cost = cost + link.Cost
                                                           });
                                    }
                                }
                            }
                            catch (Exception ex)
                            {
                                _Logger.LogException(LogLevel.Error, $"Unable to process site {site1.Name}", ex);
                            }
                        }
                    }
                }
            }

            if (hasOverridenSites)
            {
                var orderedSites = connectedSites.OrderBy(x => x.Cost);

                List<DomainController> validDcs = new List<DomainController>();

                int serversLeftToTake = maxServersToReturn;

                Dictionary<int, List<DomainController>> allDcs = new Dictionary<int, List<DomainController>>();

                foreach (Objects.ActiveDirectorySite link in orderedSites)
                {
                    try
                    {
                        List<DomainController> siteDCs = GetDomainControllersInSite(domain, link.Name, link.Cost);

                        _AllDiscoveredDomainControllers.AddRange(siteDCs);

                        Shuffle(ref siteDCs);

                        allDcs.Add(link.Cost - cost - 1, siteDCs);
                    }
                    catch (Exception ex)
                    {
                        _Logger.LogException(LogLevel.Error, $"Proccess Site - {link.Name}", ex);
                    }
                }

                int i = 0;
                int multi = 0;

                while (serversLeftToTake > 0)
                {
                    if (i > allDcs.Count - 1)
                    {
                        i = 0;
                        multi++;
                    }
                    validDcs.Add(allDcs[i][multi]);
                    serversLeftToTake--;
                    i++;            
                }

                return validDcs;
            }
            else
            {
                var orderedSites = connectedSites.OrderBy(x => x.Cost);

                List<DomainController> validDcs = new List<DomainController>();

                int serversLeftToTake = maxServersToReturn;

                foreach (Objects.ActiveDirectorySite link in orderedSites)
                {
                    try
                    {
                        List<DomainController> siteDCs = GetDomainControllersInSite(domain, link.Name, link.Cost);

                        _AllDiscoveredDomainControllers.AddRange(siteDCs);

                        if (siteDCs.Count > 0)
                        {
                            Shuffle(ref siteDCs);

                            validDcs.AddRange(siteDCs.Take(serversLeftToTake).ToList());

                            serversLeftToTake = serversLeftToTake - validDcs.Count;

                            if (serversLeftToTake <= 0)
                            {
                                return validDcs;
                            }
                        }
                    }
                    catch (Exception ex)
                    {
                        _Logger.LogException(LogLevel.Error, $"Proccess Site - {link.Name}", ex);
                    }
                }

                DirectoryContext ct = new DirectoryContext(DirectoryContextType.Forest, domain.Name);

                foreach (Objects.ActiveDirectorySite link in orderedSites)
                {
                    try
                    {
                        using (var connectedSite = ActiveDirectorySite.FindByName(ct, link.Name))
                        {
                            var sublist = GetConnectedSiteDomainControllers(domain, connectedSite, link.Cost, serversLeftToTake, level++);

                            if (sublist.Count > 0)
                            {
                                foreach (var domainController in sublist)
                                {
                                    if (validDcs.Any(x => x.IPAddress.Equals(domainController.IPAddress))) continue;

                                    validDcs.Add(domainController);
                                }
                                //validDcs.AddRange(sublist);

                                serversLeftToTake = serversLeftToTake - validDcs.Count;

                                if (serversLeftToTake <= 0)
                                {
                                    return validDcs;
                                }
                            }
                        }
                    }
                    catch (Exception ex)
                    {
                        //Console.WriteLine(ex);
                        _Logger.LogException(LogLevel.Error, $"Proccess Site Full - {link.Name}", ex);
                    }
                }

                return validDcs;
            }            
        }

        private Objects.ActiveDirectorySite GetSiteByDistinguishedName(string domainName, string distinguishedName)
        {
            if (string.IsNullOrEmpty(domainName)) throw new ArgumentNullException(nameof(domainName));
            if (string.IsNullOrEmpty(distinguishedName)) throw new ArgumentNullException(nameof(distinguishedName));

            using (LdapConnection ldap = new LdapConnection(domainName))
            {
                SearchRequest searchRequest = new SearchRequest
                                              {
                                                  DistinguishedName = distinguishedName,
                                                  Scope = SearchScope.Base,
                                                  Filter = null
                                              };

                searchRequest.Attributes.Add("name");                

                SearchResponse searchResponse = ldap.SendRequest(searchRequest) as SearchResponse;

                if (searchResponse == null) return null;

                foreach (SearchResultEntry entry in searchResponse.Entries)
                {
                    var attName = entry.Attributes.Values.Cast<DirectoryAttribute>().Where(x => "name".Equals(x.Name, StringComparison.InvariantCultureIgnoreCase)).ToList();

                    if (attName.Count == 1)
                    {
                        object[] rawValues = attName[0].GetValues(typeof(string));

                        if (rawValues.Length == 0) return null;

                        var values = rawValues.Where(x => x != null).Cast<string>().ToList();

                        if (values.Count == 0) return null;

                        return new Objects.ActiveDirectorySite
                               {
                                   Name = values[0],
                                   DistinguishedName = distinguishedName
                               };
                    }
                }
            }

            return null;
        }

        private List<Objects.SiteOverrideData> GetOverridenSites(string domainName, string siteDistinguishedName)
        {
            if (string.IsNullOrEmpty(siteDistinguishedName)) throw new ArgumentNullException(nameof(siteDistinguishedName));

            List<Objects.SiteOverrideData> sites = new List<Objects.SiteOverrideData>();

            using (LdapConnection ldap = new LdapConnection(domainName))
            {
                SearchRequest searchRequest = new SearchRequest
                                              {
                                                  DistinguishedName = siteDistinguishedName,
                                                  Scope = SearchScope.Base,
                                                  Filter = null
                                              };

                searchRequest.Attributes.Add("url");

                SearchResponse searchResponse = ldap.SendRequest(searchRequest) as SearchResponse;

                if (searchResponse == null) return null;

                foreach (SearchResultEntry entry in searchResponse.Entries)
                {
                    var attUrl = entry.Attributes.Values.Cast<DirectoryAttribute>().Where(x => "url".Equals(x.Name, StringComparison.InvariantCultureIgnoreCase)).ToList();

                    if (attUrl.Count == 1)
                    {                        
                        object[] rawValues = attUrl[0].GetValues(typeof(string));

                        if (rawValues.Length == 0) return null;

                        var values = rawValues.Where(x => x != null).Cast<string>().ToList();

                        if (values.Count == 0) return null;

                        foreach (var value in values)
                        {
                            string[] data = value.Split('|');

                            if (data.Length == 3)
                            {
                                try
                                {
                                    var overrideType = (SiteOverrideType)Enum.Parse(typeof(SiteOverrideType), data[0], true);

                                    int order;

                                    if (int.TryParse(data[2], out order))
                                    {
                                        sites.Add(new SiteOverrideData
                                                  {
                                                      Type = overrideType,
                                                      DistinguishedName = data[1],
                                                      Order = order
                                                  });
                                    }                                    
                                }
                                catch
                                {                                    
                                }                                
                            }
                        }
                    }                    
                }
            }

            return sites;
        }

        private List<DomainController> GetDomainControllersInSite(Domain domain, string siteName, int order)
        {
            List<DomainController> domainControllers = new List<DomainController>();

            var siteDCs = domain.FindAllDomainControllers(siteName);

            foreach (System.DirectoryServices.ActiveDirectory.DomainController controller in siteDCs)
            {
                try
                {
                    using (controller)
                    {
                        string ipaddress;
                        if ("::1".Equals(controller.IPAddress))
                        {
                            try
                            {
                                ipaddress = GetLocalIPAddress();
                            }
                            catch (Exception ex1)
                            {
                                _Logger.LogException(LogLevel.Error, "Unable to find local IP address", ex1);
                                ipaddress = "127.0.0.1";
                            }                            
                        }
                        else
                        {
                            ipaddress = controller.IPAddress;
                        }

                        if (CheckIfServerIsRunningDNS(ipaddress))
                        {
                            DomainController dc = new DomainController
                            {
                                DomainName = domain.Name,
                                FullyQualifyedServerName = controller.Name,
                                Name = controller.Name,
                                SiteName = controller.SiteName,
                                IPAddress = ipaddress,
                                IsDNSServer = true,
                                Order = order++
                            };
                                                        
                            domainControllers.Add(dc);
                        }                        
                    }
                }
                catch (Exception ex)
                {
                    _Logger.LogException(LogLevel.Error, "Proccess Domain Controler", ex);
                }
            }

            return domainControllers;
        }

        private bool CheckIfServerIsRunningDNS(string ipAddress)
        {
            if (string.IsNullOrEmpty(ipAddress))
            {
                _Logger.Error($"Unable to check if corporate network server {ipAddress} is DNS Server");
                return false;
            }

            bool connectSuccess = false;
            try
            {
                for (int j = 1; j <= 3; j++)
                {
                    using (TcpClient tcpScan = new TcpClient())
                    {
                        tcpScan.BeginConnect(ipAddress, 53, null, null);
                        for (int i = 1; i <= 10; i++)
                        {
                            Thread.Sleep(200);
                            if (tcpScan.Connected)
                            {
                                connectSuccess = true;
                                //tcpScan.Close();
                                break;
                            }

                            if (i == 10)
                            {
                                tcpScan.Close();
                            }
                        }

                        if (connectSuccess)
                        {
                            return true;
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                _Logger.LogException(LogLevel.Error, string.Format("Unable to check if corporate network server {0} is DNS Server", ipAddress), ex);
            }
            return false;
        }

        public string GetLocalIPAddress()
        {            
            using (Socket socket = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, 0))
            {
                socket.Connect("8.8.8.8", 53);
                IPEndPoint endPoint = socket.LocalEndPoint as IPEndPoint;

                if (endPoint == null) return GetLocalIPAddress2();

                return endPoint.Address.ToString();
            }
        }

        public string GetLocalIPAddress2()
        {
            IPHostEntry host = Dns.GetHostEntry(Dns.GetHostName());

            return host.AddressList.FirstOrDefault(ip => ip.AddressFamily == AddressFamily.InterNetwork).ToString();
        }

        private void Shuffle(ref List<DomainController> list)
        {
            Random rng = new Random();

            int n = list.Count;
            while (n > 1)
            {
                n--;
                int k = rng.Next(n + 1);
                DomainController value = list[k];
                list[k] = list[n];
                list[n] = value;
            }
        }
    }
}