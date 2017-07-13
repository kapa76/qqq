using System;
using System.Collections.Generic;
using System.DirectoryServices.ActiveDirectory;
using System.DirectoryServices.Protocols;
using System.Linq;
using System.Text;
using NLog;
using Synergix.ADCE.Lite.Commands;
using Synergix.ADCE.Lite.Objects;
using ActiveDirectorySite = System.DirectoryServices.ActiveDirectory.ActiveDirectorySite;
using ActiveDirectorySiteLink = System.DirectoryServices.ActiveDirectory.ActiveDirectorySiteLink;
using DomainController = Synergix.ADCE.Lite.Objects.DomainController;

namespace Synergix.ADCE.Lite.Providers
{
    public class WINSServerProvider : BaseProvider
    {
        private readonly CommandContext _Context;
        private readonly Logger _Logger;
        private readonly List<string> _CheckedSites;

        public WINSServerProvider(Options options, CommandContext context) : base(options)
        {
            if (context == null) throw new ArgumentNullException(nameof(context));

            _Context = context;
            _Logger = LogManager.GetCurrentClassLogger();
            AllDiscoveredWINSServers = new List<WindowsServer>();
            _CheckedSites = new List<string>();
        }

        public List<WindowsServer> AllDiscoveredWINSServers { get; }

        public IList<WindowsServer> GetWINSServersFromClosesedSite(int maxServersToReturn)
        {
            int maxServers = maxServersToReturn;

            using (var currentComputerSite = ActiveDirectorySite.GetComputerSite())
            {
                using (var currentDomain = Domain.GetComputerDomain())
                {
                    List<WindowsServer> servers = new List<WindowsServer>();

                    servers.AddRange(GetWINSServers(currentDomain, currentComputerSite, 0, maxServers, 1));

                    return servers;
                }
            }
        }

        private IList<WindowsServer> GetWINSServers(Domain domain, ActiveDirectorySite site, int cost, int maxServersToReturn, int level)
        {
            if (level > 3) return new List<WindowsServer>();

            List<WindowsServer> servers = new List<WindowsServer>();
            string siteDn = string.Empty;

            using (var siteDe = site.GetDirectoryEntry())
            {
                try
                {
                    var siteDnObj = siteDe.Properties["distinguishedName"];

                    if (siteDnObj?.Value != null)
                    {
                        siteDn = siteDnObj.Value.ToString();
                    }
                }
                catch (Exception ex)
                {
                    _Logger.LogException(LogLevel.Error, $"Unable to process site {site.Name}", ex);
                }
            }

            if (!string.IsNullOrEmpty(siteDn))
            {
                var overridenSitesList = GetOverridenSites(domain.Name, siteDn);

                if (overridenSitesList != null)
                {
                    var overridenSites = overridenSitesList.Where(x => x.Type == SiteOverrideType.WINS).ToList();

                    if (overridenSites.Count > 0)
                    {
                        List<WindowsServer> siteServers = new List<WindowsServer>();

                        foreach (var overridenSite in overridenSites)
                        {                            
                            WindowsServer server = new WindowsServer
                            {
                                DomainName = domain.Name,
                                IPAddress = overridenSite.DistinguishedName,
                                IsWINSServer = true,
                                SiteName = site.Name,
                                Order = cost++
                            };

                            siteServers.Add(server);
                        }

                        servers.AddRange(siteServers.Take(maxServersToReturn).ToList());

                        if (servers.Count == maxServersToReturn) return servers;
                    }
                }
            }

            //TODO: Odi kaj site links

            _CheckedSites.Add(site.Name);

            int maxToReturn = maxServersToReturn - servers.Count;

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

                    var single = servers.SingleOrDefault(x => x.Name.Equals(site1.Name, StringComparison.InvariantCultureIgnoreCase));
                    if (single == null)
                    {
                        servers.AddRange(GetWINSServers(domain, site1, cost + link.Cost, maxToReturn, level++));

                        if (servers.Count == maxServersToReturn) return servers;

                        maxToReturn = maxToReturn - servers.Count;
                    }
                }
            }

            return servers;
        }

        private List<SiteOverrideData> GetOverridenSites(string domainName, string siteDistinguishedName)
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
    }
}
