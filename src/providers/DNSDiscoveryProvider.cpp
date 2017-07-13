using System;
using System.Collections.Generic;
using System.DirectoryServices.ActiveDirectory;
using System.DirectoryServices.Protocols;
using System.Linq;
using System.Text;
using NLog;
using Synergix.ADCE.Lite.Commands;
using Synergix.ADCE.Lite.Core;
using Synergix.ADCE.Lite.Objects;
using ActiveDirectorySite = Synergix.ADCE.Lite.Objects.ActiveDirectorySite;
using ActiveDirectorySiteLink = Synergix.ADCE.Lite.Objects.ActiveDirectorySiteLink;

namespace Synergix.ADCE.Lite.Providers
{
    public class DNSDiscoveryProvider : BaseProvider
    {
        private readonly CommandContext _Context;
        private readonly Logger _Logger;

        public DNSDiscoveryProvider(Options options, CommandContext context) : base(options)
        {
            if (options == null) throw new ArgumentNullException(nameof(options));
            if (context == null) throw new ArgumentNullException(nameof(context));

            _Context = context;
            _Logger = LogManager.GetCurrentClassLogger();
        }

        public List<ValidDNS> Discover()
        {
            List<ValidDNS> newValidDnsList = new List<ValidDNS>();

            try
            {
                var lisValidDNS = GetCurrentComputerClosestSiteDNSServers();
            }
            catch (Exception ex)
            {
                Console.ResetColor();
                Console.WriteLine("Log information is stored in the file: {0}", Options.LogFilePath);

                _Logger.LogException(LogLevel.Error, "GetList", ex);
            }

            return newValidDnsList;
        }

        private List<ValidDNS> GetCurrentComputerClosestSiteDNSServers()
        {
            var servers = new List<ValidDNS>();

            int serversInPriorityDomainSite = 0;

            try
            {
                List<Objects.ActiveDirectorySiteLink> siteLinksChecked = new List<Objects.ActiveDirectorySiteLink>();
                List<Objects.ActiveDirectorySite> allNeededSites = new List<Objects.ActiveDirectorySite>
                                                           {
                                                               _Context.ComputerActiveDirectorySite
                                                           };

                var allSiteLinks = GetSiteLinks(GetSitePath(_Context.ComputerActiveDirectorySite.DistinguishedName));

                if (allSiteLinks.Count == 0)
                {
                    Console.ResetColor();
                    Console.WriteLine("Site [{0}] is not a member of any Site Link.", _Context.ComputerActiveDirectorySite.Name);

                    _Logger.Warn("Site [{0}] is not a member of any Site Link.", _Context.ComputerActiveDirectorySite.Name);
                }


                using (LdapConnection ldapConnectionHostDomain = new LdapConnection(_Context.DomainName))
                {                    
                    if (GlobalData.MaximalSiteLevel > 0)
                    {
                        GetSitesToCheck(_Context.ComputerActiveDirectorySite, 1, ref allNeededSites, ref siteLinksChecked, allSiteLinks);
                    }


                    //   allSitesOrdered = allSitesOrdered.OrderBy(s => s.Cost).ThenBy(s => s.Level).ThenBy(s => s.Name).ToList();

                    //LINQ
                    // allNeededSitesOrdered = allNeededSites.OrderBy(s => s.Cost).ThenBy(s => s.Level).ThenBy(s => s.Name).ToList();

                    allNeededSites.Sort((x, y) => x.Cost.CompareTo(y.Cost));

                    int oldCost = -1; //-1 because it will compare -1 != 0 , in first casenbcxjhasiisism


                    foreach (Objects.ActiveDirectorySite site in allNeededSites)
                    {
                        if ((oldCost != -1) && (oldCost != site.Cost)) //site with new higher cost then previous is reached
                        {
                            if ((serversInPriorityDomainSite >= GlobalData.MaximumDNSServers))
                            {
                                Console.ResetColor();
                                Console.WriteLine("Maximum number of [{0}] DCs detected after reading site group with cost {1}", GlobalData.MaximumDNSServers, oldCost);

                                _Logger.Info("Maximum number of [{0}] DCs is reached after reading site group with cost [{1}]", GlobalData.MaximumDNSServers, oldCost);

                                return servers;
                            }
                        }

                        //logic is - for each site, phase number is increasing


                       // GetSiteServersWithLog(site, ref servers, ref serversInPriorityDomainSite, ldapConnectionHostDomain);

                        oldCost = site.Cost;
                    }
                }
            }
            catch (Exception ex)
            {
                Console.ResetColor();
                Console.WriteLine("Log information is stored in the file: {0}", Options.LogFilePath);

                _Logger.LogException(LogLevel.Error, "GetCurrentComputerClosestSiteDNSServers", ex);
            }

            return servers;
        }

        private List<Objects.ActiveDirectorySiteLink> GetSiteLinks(string siteDistinguishedName)
        {
            if (string.IsNullOrEmpty(siteDistinguishedName)) throw new ArgumentNullException(nameof(siteDistinguishedName));

            List<Objects.ActiveDirectorySiteLink> siteLinks = new List<Objects.ActiveDirectorySiteLink>();

            try
            {
                using (LdapConnection ldap = new LdapConnection(_Context.DomainName))
                {
                    List<string> attributes = new List<string>
                                              {
                                                  "name",
                                                  "cost",
                                                  "siteList",
                                                  "distinguishedName"
                                              }; 

                    SearchRequest searchRequest = new SearchRequest(siteDistinguishedName, "objectClass=siteLink", SearchScope.Subtree, attributes.ToArray());

                    SearchResponse searchResponse = ldap.SendRequest(searchRequest) as SearchResponse;

                    if (searchResponse != null)
                    {
                        foreach (SearchResultEntry entry in searchResponse.Entries)
                        {
                            List<Objects.ActiveDirectorySite> siteLinkSites = new List<Objects.ActiveDirectorySite>();

                            var siteList = entry.Attributes["sitelist"];

                            foreach (var site in siteList)
                            {
                                string siteDn = site.ToString();

                                siteLinkSites.Add(new Objects.ActiveDirectorySite
                                                                 {
                                                                     DistinguishedName = siteDn,
                                                                     Name = GetNameFromDistinguishedName(siteDn)
                                                                 });
                            }

                            string stringValue = entry.Attributes["cost"][0].ToString();

                            int intValue;
                            int.TryParse(stringValue, out intValue);

                            Objects.ActiveDirectorySiteLink activeDirectorySiteLink = new Objects.ActiveDirectorySiteLink
                                                                              {
                                                                                  ActiveDirectorySiteArray = siteLinkSites,
                                                                                  Cost = intValue,
                                                                                  Name = entry.Attributes["name"][0].ToString(),
                                                                                  DistinguishedName = entry.Attributes["distinguishedName"][0].ToString()
                                                                              };

                            siteLinks.Add(activeDirectorySiteLink);
                        }
                    }
                    else
                    {
                        _Logger.Warn("No Active Directory site links found");
                    }
                }
            }
            catch (Exception ex)
            {
                _Logger.LogException(LogLevel.Error, $"Unable to get site links for {siteDistinguishedName}", ex);
            }

            return siteLinks;
        }

        private void GetSitesToCheck(Objects.ActiveDirectorySite currentSite, int siteLevel, ref List<Objects.ActiveDirectorySite> sitesAll, ref List<Objects.ActiveDirectorySiteLink> siteLinksChecked, List<Objects.ActiveDirectorySiteLink> adSiteLinksAll)
        {
            List<Objects.ActiveDirectorySiteLink> currentSiteLinkList2 = new List<Objects.ActiveDirectorySiteLink>();
            try
            {
                //LINQ
                //foreach (Command.ActiveDirectorySiteLink adSiteLink in adSiteLinksAll.Where(t => t.ActiveDirectorySiteArray.Any(s => s.DistinguishedName.Equals(currentSite.DistinguishedName, StringComparison.InvariantCultureIgnoreCase))))
                //{
                //    currentSiteLinkList2.Add(adSiteLink);
                //}
                foreach (Objects.ActiveDirectorySiteLink adSiteLink in adSiteLinksAll)
                {
                    foreach (Objects.ActiveDirectorySite adSite in adSiteLink.ActiveDirectorySiteArray)
                    {
                        if (adSite.DistinguishedName.Equals(currentSite.DistinguishedName, StringComparison.InvariantCultureIgnoreCase))
                        {
                            currentSiteLinkList2.Add(adSiteLink);
                            break;
                        }
                    }
                }


                List<Objects.ActiveDirectorySite> tempSiteList = new List<Objects.ActiveDirectorySite>();

                //this second foreach is because of OrderBy, it can happen a site to be in more than one site links, and with this we will choose site link with lower cost
                currentSiteLinkList2.Sort((x, y) => x.Cost.CompareTo(y.Cost));

                //LINQ
                //  foreach (Command.ActiveDirectorySiteLink siteLink in currentSiteLinkList2.OrderBy(t => t.Cost))
                foreach (Objects.ActiveDirectorySiteLink siteLink in currentSiteLinkList2)
                {
                    if (string.IsNullOrEmpty(siteLink.Name)) continue;
                    //  //if (siteLink.Name.Equals("DEFAULTIPSITELINK", StringComparison.InvariantCultureIgnoreCase)) continue;

                    //LINQ
                    // if (siteLinksChecked.Any(s => s.Name == siteLink.Name)) continue;

                    bool found = false;
                    foreach (var sl in siteLinksChecked)
                    {
                        if (sl.Name.Equals(siteLink.Name, StringComparison.InvariantCultureIgnoreCase))
                        {
                            found = true;
                            break;
                        }
                    }

                    if (found) continue;

                    siteLinksChecked.Add(siteLink);

                    foreach (Objects.ActiveDirectorySite childSite in siteLink.ActiveDirectorySiteArray)
                    {
                        if (currentSite.DistinguishedName.Equals(childSite.DistinguishedName, StringComparison.InvariantCultureIgnoreCase))
                        {
                            continue;
                        }

                        //LINQ
                        // if (sitesAll.Any(s => s.DistinguishedName.Equals(childSite.DistinguishedName, StringComparison.InvariantCultureIgnoreCase))) continue;


                        foreach (var site in sitesAll)
                        {
                            if (site.DistinguishedName.Equals(childSite.DistinguishedName, StringComparison.InvariantCultureIgnoreCase))
                            {
                                found = true;
                                break;
                            }
                        }
                        if (found) continue;

                        //INCLUDE also site links cout!!!!

                        //LINQ
                        //int sitelikscount = adSiteLinksAll.Count(sl => sl.ActiveDirectorySiteArray.Any(st => st.Name.Equals(childSite.Name)));

                        int sitelikscount = 0;
                        foreach (var sl in adSiteLinksAll)
                        {
                            foreach (var adSite in sl.ActiveDirectorySiteArray)
                            {
                                if (adSite.Name.Equals(childSite.Name, StringComparison.InvariantCultureIgnoreCase))
                                {
                                    sitelikscount++;
                                    break;
                                }
                            }
                        }

                        Objects.ActiveDirectorySite childActiveDirectorySite = new Objects.ActiveDirectorySite { SiteLinksCount = sitelikscount, Cost = siteLink.Cost + currentSite.Cost, Name = childSite.Name, SiteLinks = null, Level = siteLevel, DistinguishedName = childSite.DistinguishedName };

                        sitesAll.Add(childActiveDirectorySite);
                        tempSiteList.Add(childActiveDirectorySite);
                    }
                }

                if ((siteLevel >= GlobalData.MaximalSiteLevel) || (sitesAll.Count >= GlobalData.MaximumSites)) return;
            
                siteLevel += 1;


                foreach (Objects.ActiveDirectorySite adSite in tempSiteList)
                {
                    GetSitesToCheck(adSite, siteLevel, ref sitesAll, ref siteLinksChecked, adSiteLinksAll);
                }
            }
            catch (Exception ex)
            {
                Console.ResetColor();
                Console.WriteLine("Log information is stored in the file: {0}", Options.LogFilePath);

                _Logger.LogException(LogLevel.Error, "GetSitesToCheck", ex);
            }
        }

        private string GetNameFromDistinguishedName(string identity)
        {
            if (identity.Contains(","))
            {
                string[] identityList = identity.Split(',');
                int index1 = identityList[0].IndexOf("N=", StringComparison.InvariantCulture);
                if (index1 != 0)
                {
                    string result = string.Format(identityList[0].Substring(index1 + 2));
                    return result;
                }
                return identityList[0];
            }
            return identity;
        }

        private string GetSitePath(string identity)
        {
            int index1 = identity.IndexOf(",CN=Sites", StringComparison.InvariantCulture);
            if (index1 != 0)
            {
                string result = string.Format(identity.Substring(index1 + 1));
                return result;
            }
            return identity;
        }

        public List<Objects.ActiveDirectorySite> GetSites()
        {
            List<Objects.ActiveDirectorySite> sites = new List<Objects.ActiveDirectorySite>();



            return sites;
        }

        private List<Objects.ActiveDirectorySite> GetSites(Objects.ActiveDirectorySite site)
        {
            List<Objects.ActiveDirectorySite> sites = new List<Objects.ActiveDirectorySite> {site};

            

            //Get site links
            //First check if there is owerride
            //else follow site links

            var t = System.DirectoryServices.ActiveDirectory.ActiveDirectorySite.FindByName(new DirectoryContext(DirectoryContextType.DirectoryServer), "");

          //  t.SiteLinks[0].

            //sites.AddRange(GetSites());

            return sites;
        }

        //private List<Objects.ActiveDirectorySite> GetOverridenSites(string siteDistinguishedName)
        //{
        //    if (string.IsNullOrEmpty(siteDistinguishedName)) throw new ArgumentNullException(nameof(siteDistinguishedName));

        //    using (LdapConnection connection = new LdapConnection(_Context.DomainName))
        //    {
        //        SearchRequest searchRequest = new SearchRequest
        //                                      {
        //                                          DistinguishedName = siteDistinguishedName,
        //                                          Scope = SearchScope.Base,
        //                                          Filter = null
        //                                      };

        //        searchRequest.Attributes.Add("url");

        //        SearchResponse searchResponse = connection.SendRequest(searchRequest) as SearchResponse;

        //        if (searchResponse == null) return null;

        //        foreach (SearchResultEntry entry in searchResponse.Entries)
        //        {
        //            List<string> urlValues = GetStringAttributeValue(entry.Attributes, "url");

        //            if (urlValues == null) return null;

        //            break;
        //        }
        //    }
        //}

        internal static List<string> GetStringAttributeValue(SearchResultAttributeCollection attributes, string attributeName)
        {
            if (attributes == null) throw new ArgumentNullException(nameof(attributes));
            if (string.IsNullOrEmpty(attributeName)) throw new ArgumentNullException(nameof(attributeName));

            if (attributes.Values == null) return null;

            foreach (DirectoryAttribute attribute in attributes.Values)
            {
                if (!attribute.Name.Equals(attributeName, StringComparison.InvariantCultureIgnoreCase)) continue;

                object[] vals = attribute.GetValues(typeof(string));

                if (vals.Length == 0) return null;

                return vals.Select(x => x.ToString()).ToList();                
            }

            return null;
        }
    }
}
