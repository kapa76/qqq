using System;
using System.Collections.Generic;
using System.DirectoryServices.Protocols;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using NLog;
using Synergix.ADCE.Lite.Core;
using Synergix.ADCE.Lite.Objects;


namespace Synergix.ADCE.Lite
{
    public class DnsDiscovery
    {
        private static readonly Logger Logger = LogManager.GetCurrentClassLogger();

        public static List<ValidDNS> GetList()
        {
            List<ValidDNS> newValidDnsList = new List<ValidDNS>();

            try
            {
                var lisValidDNS = GetCurrentComputerClosestSiteDNSServers(false);


                if (lisValidDNS.Count > 0)
                {
                    //THIS makes ORDER servers properly and fill value ORDER

                    //groupby with 2 levels of grouping, this is needed for load balancing and check dns in nic optimality
                    //LINQ
                    //var queryNestedGroups =


                    //    lisValidDNS.OrderBy(server => server.Cost)
                    //        .ThenByDescending(server => server.RatingMark)
                    //        .ThenBy(server => server.SiteLevel)
                    //        .ThenBy(server => server.ServerName)
                    //        .GroupBy(server => server.Cost)
                    //        .SelectMany(newGroup1 => (from server in newGroup1
                    //            group server by server.RatingMark), (newGroup1, newGroup2) => new {newGroup1, newGroup2})
                    //        .GroupBy(@t => newGroup1.Key, @t => newGroup2);


                    // lisValidDNS.Sort((x, y) => x.Cost.CompareTo(y.Cost));
                    lisValidDNS.Sort((x, y) => x.RatingMark.CompareTo(y.RatingMark));
                    lisValidDNS.Reverse();

                    //LINQ groupby translated to regular code
                    //NOT needed, in this app, because there is no load balancing, and ordering by other parameters

                    //    List<List<Command.ValidDNS>> groupByRatingMarkList = new List<List<Command.ValidDNS>>();

                    //    int previousRatingMark = 100000;
                    //     foreach (var validDNS in lisValidDNS)
                    //    {

                    //        int currentRatingMark = validDNS.RatingMark;
                    //        if (previousRatingMark == 100000)  //first element in the list
                    //        {
                    //            previousRatingMark = currentRatingMark;
                    //            newValidDnsList.Add(validDNS);
                    //        }
                    //        else //rest of the list
                    //        {
                    //            if (currentRatingMark == previousRatingMark)  //still fill into same group
                    //            {
                    //                newValidDnsList.Add(validDNS);
                    //            }

                    //            else
                    //            {
                    //                groupByRatingMarkList.Add(newValidDnsList);
                    //                previousRatingMark = currentRatingMark;
                    //                newValidDnsList = new List<Command.ValidDNS>();
                    //                newValidDnsList.Add(validDNS);
                    //            }

                    //        }
                    //    }

                    ////after looping all dns servers
                    //    if (newValidDnsList.Count > 0)
                    //    {
                    //        groupByRatingMarkList.Add(newValidDnsList);
                    //    }


                    int order = 1;
                    bool counterOrderIncrease = false;
                    foreach (var validDNS in lisValidDNS)
                    {
                        var newServer = validDNS;
                        if (newServer.IsValid)
                        {
                            newServer.Order = order;
                            counterOrderIncrease = true;
                        }
                        else
                        {
                            newServer.Order = 999999;
                        }

                        newValidDnsList.Add(newServer);

                        if (counterOrderIncrease)
                        {
                            order++;
                            counterOrderIncrease = false;
                        }
                    }
                }
                else 
                {
                    // THERE IS NOTHING IN THE LIST OF SERVERS
                    
                    Console.ResetColor();
                    Console.WriteLine("List of valid DNS servers is empty");

                    Logger.Warn("List of valid DNS servers is empty");                    
                }
            }
            catch (Exception ex)
            {                
                Console.ResetColor();
                Console.WriteLine("Log information is stored in the file: {0}", GlobalData.LogFilePath);                

                Logger.LogException(LogLevel.Error, "GetList", ex);
            }
            return newValidDnsList;
        }

        #region "private methodes"

        public static int PingReplyMarkCalculator(int pingReplay)
        {
            if ((pingReplay >= 0) && (pingReplay < 50))
            {
                return (int) PingReplyMark.HIGH;
            }
            if ((pingReplay >= 50) && (pingReplay < 100))
            {
                return (int) PingReplyMark.MEDIUM;
            }
            if ((pingReplay >= 100) && (pingReplay < 500))
            {
                return (int) PingReplyMark.LOW;
            }
            if (pingReplay >= 500)
            {
                return (int) PingReplyMark.BAD;
            }
            return (int) PingReplyMark.NOT_AVAILABLE;
        }


        private static List<ActiveDirectorySiteLink> GetAdSiteLinks(string siteDistinguishedName, LdapConnection ldapConnection)
        {
            if (string.IsNullOrEmpty(siteDistinguishedName)) throw new ArgumentNullException("siteDistinguishedName");
            List<ActiveDirectorySiteLink> siteLinks = new List<ActiveDirectorySiteLink>();

            try
            {
                SearchRequest searchRequest = new SearchRequest(siteDistinguishedName, "objectClass=siteLink", SearchScope.Subtree, "name", "cost", "siteList", "distinguishedName");

                SearchResponse searchResponse = ldapConnection.SendRequest(searchRequest) as SearchResponse;

                if (searchResponse != null)
                {
                    foreach (SearchResultEntry entry in searchResponse.Entries)
                    {
                        ActiveDirectorySiteLink obj = new ActiveDirectorySiteLink();
                        ActiveDirectorySite[] tempActiveDirectorySiteArray = new ActiveDirectorySite[entry.Attributes["sitelist"].Count];

                        for (int i = 0; i < tempActiveDirectorySiteArray.Length; i++)
                        {
                            var adSite = new ActiveDirectorySite {DistinguishedName = entry.Attributes["sitelist"][i].ToString()};
                            adSite.Name = GetNameFromDistinguishedName(adSite.DistinguishedName);
                            tempActiveDirectorySiteArray[i] = adSite;
                        }

                        string stringValue = entry.Attributes["cost"][0].ToString();

                        int intValue;
                        int.TryParse(stringValue, out intValue);

                        obj.ActiveDirectorySiteArray = tempActiveDirectorySiteArray;
                        obj.Cost = intValue;
                        obj.Name = entry.Attributes["name"][0].ToString();
                        obj.DistinguishedName = entry.Attributes["distinguishedName"][0].ToString();


                        siteLinks.Add(obj);
                    }
                }
                else
                {
                    Logger.Warn("No Active Directory site links found");
                }
            }

            catch (Exception ex)
            {
                Logger.LogException(LogLevel.Error, "GetAdSiteLinks", ex);
            }

            return siteLinks;
        }


        private static List<ValidDNS> GetCurrentComputerClosestSiteDNSServers(bool dnsFullScan)
        {
            var servers = new List<ValidDNS>();
            var allSiteLinks = new List<ActiveDirectorySiteLink>();

            int serversInPriorityDomainSite = 0;
            //  allSitesOrdered = new List<Command.ActiveDirectorySite>();
            //var allNeededSitesOrdered = new List<Command.ActiveDirectorySite>();

            try
            {
                using (LdapConnection ldapConnectionHostDomain = new LdapConnection(GlobalData.HostDomain.Name))
                {
                    ActiveDirectorySite activeDirectoryHostSite = new ActiveDirectorySite
                                        {
                                            SiteLinksCount = GlobalData.MachineActiveDirectorySite.SiteLinks.Count,
                                            Cost = 0,
                                            Name = GlobalData.MachineActiveDirectorySite.Name,
                                            SiteLinks = GlobalData.MachineActiveDirectorySite.SiteLinks,
                                            Level = 0,
                                            DistinguishedName = GlobalData.MachineAdSiteDistinguishedName
                                        };

                    List<ActiveDirectorySite> allNeededSites = new List<ActiveDirectorySite>();
                    List<ActiveDirectorySiteLink> siteLinksChecked = new List<ActiveDirectorySiteLink>();
                    allNeededSites.Add(activeDirectoryHostSite);
                    

                    allSiteLinks = GetAdSiteLinks(GetSitePath(GlobalData.MachineAdSiteDistinguishedName), ldapConnectionHostDomain);

                    if (activeDirectoryHostSite.SiteLinksCount == 0)
                    {
                        Console.ResetColor();
                        Console.WriteLine("Site [{0}] is not a member of any Site Link.", activeDirectoryHostSite.Name);

                        Logger.Warn("Site [{0}] is not a member of any Site Link.", activeDirectoryHostSite.Name);
                    }

                    if (GlobalData.MaximalSiteLevel > 0)
                    {
                        GetSitesToCheck(false, activeDirectoryHostSite, false, 1, ref allNeededSites, ref siteLinksChecked, allSiteLinks);
                    }


                    //   allSitesOrdered = allSitesOrdered.OrderBy(s => s.Cost).ThenBy(s => s.Level).ThenBy(s => s.Name).ToList();

                    //LINQ
                    // allNeededSitesOrdered = allNeededSites.OrderBy(s => s.Cost).ThenBy(s => s.Level).ThenBy(s => s.Name).ToList();

                    allNeededSites.Sort((x, y) => x.Cost.CompareTo(y.Cost));

                    int oldCost = -1; //-1 because it will compare -1 != 0 , in first casenbcxjhasiisism


                    foreach (ActiveDirectorySite site in allNeededSites)
                    {
                        if ((oldCost != -1) && (oldCost != site.Cost)) //site with new higher cost then previous is reached
                        {
                            if (!dnsFullScan && (serversInPriorityDomainSite >= GlobalData.MaximumDNSServers))
                            {                                
                                Console.ResetColor();
                                Console.WriteLine("Maximum number of [{0}] DCs detected after reading site group with cost {1}", GlobalData.MaximumDNSServers, oldCost);                                

                                Logger.Info("Maximum number of [{0}] DCs is reached after reading site group with cost [{1}]", GlobalData.MaximumDNSServers, oldCost);
                                
                                return servers;
                            }
                        }

                        //logic is - for each site, phase number is increasing
                        GetSiteServersWithLog(site, ref servers, ref serversInPriorityDomainSite, ldapConnectionHostDomain);

                        oldCost = site.Cost;
                    }
                }
            }
            catch (Exception ex)
            {                
                Console.ResetColor();
                Console.WriteLine("Log information is stored in the file: {0}", GlobalData.LogFilePath);
                
                Logger.LogException(LogLevel.Error, "GetCurrentComputerClosestSiteDNSServers", ex);
            }

            return servers;
        }


        private static void GetSiteServersWithLog(ActiveDirectorySite site, ref List<ValidDNS> serversFull, ref int serversInPriorityDomainSite, LdapConnection ldapConnectionHostDomain)
        {
            if (string.IsNullOrEmpty(site.DistinguishedName)) return;
            
            var siteServers = GetServersForSite(site.DistinguishedName, ldapConnectionHostDomain);

            if (siteServers.Count != 0)
            {
                foreach (ValidDNS srv in siteServers)
                {
                    ValidDNS server = srv;
                    try
                    {
                        //  ComputerId will be read from ValidDNSList.ComputerId, it is foreign key, there were problems if you add ComputerId here
                        server.OperatingSystem = string.Empty;
                        server.OperatingSystemServicePack = string.Empty;

                        bool isOnHostSite = false;
                        bool isOnHostDomain = false;
                        bool isOnHostParentDomain = false;

                        int ratingMark = 0;

                        //LINQ
                        //if (serversFull.Any(addedServer => server.ServerName.Equals(addedServer.ServerName, StringComparison.InvariantCultureIgnoreCase))) continue;

                        bool allreadyInTheList = false;
                        foreach (var newServer in serversFull)
                        {
                            if (server.ServerName.Equals(newServer.ServerName, StringComparison.InvariantCultureIgnoreCase))
                            {
                                allreadyInTheList = true;
                                break;
                            }
                        }

                        if (allreadyInTheList) continue;

                        //ref server, true, false (to use AD value because is for database records)
                        Command.CheckValidityOfValidDNSReduced(ref server);

                        if (!server.IsValid) continue;

                        if (GlobalData.HostDomain.Name.Equals(server.DomainName, StringComparison.InvariantCultureIgnoreCase))
                        {
                            isOnHostDomain = true;
                            ratingMark += GlobalData.RatingMarkDomain;
                            if (GlobalData.DomainPriority)
                            {
                                serversInPriorityDomainSite++;
                            }
                        }
                        else
                        {
                            //todo: maybe add another atribute for this ???
                            //server.IsValid = false; //SET dns members of other domani than current as invalid. exclude from list

                            if (!Program.IsForestRoot)
                            {
                                if (Program.ParentDomainName.Equals(server.DomainName, StringComparison.InvariantCultureIgnoreCase) && !string.IsNullOrEmpty(server.DomainName))
                                {
                                    isOnHostParentDomain = true;
                                    ratingMark += GlobalData.RatingMarkParentDomain; // server belongs to partent domain of host domain  
                                }
                            }
                        }

                        if (GlobalData.MachineActiveDirectorySite.Name.Equals(site.Name, StringComparison.InvariantCultureIgnoreCase))
                        {
                            isOnHostSite = true;
                            ratingMark += GlobalData.RatingMarkSite;
                            if (!GlobalData.DomainPriority)
                            {
                                serversInPriorityDomainSite++;
                            }
                        }

                        if (server.FqServerName.Equals(GlobalData.MachineFqName, StringComparison.InvariantCultureIgnoreCase)) ratingMark += GlobalData.RatingMarkIsSelf;

                        ratingMark += PingReplyMarkCalculator(server.PingReply);

                        server.IsOnHostDomain = isOnHostDomain;
                        server.IsOnHostParentDomain = isOnHostParentDomain;
                        server.IsOnHostSite = isOnHostSite;
                        server.SiteName = site.Name;
                        server.Cost = site.Cost;
                        server.SiteLevel = site.Level;
                        server.SiteLinksCount = site.SiteLinksCount;

                        switch (server.SiteLinksCount)
                        {
                            case 0:
                                server.Note += string.Format("Site [{0}] is not a member of any Site Link. ", server.SiteName);
                                break;
                        }

                        if (server.IsValid)
                        {
                            server.RatingMark = ratingMark;
                            serversFull.Add(server);
                        }
                        else
                        {
                            server.RatingMark = 0;
                        }
                    }
                    catch (Exception ex)
                    {
                        Console.ResetColor();
                        Console.WriteLine("Log information is stored in the file: {0}", GlobalData.LogFilePath);

                        Logger.LogException(LogLevel.Error, "GetSiteServersWithLog", ex);
                    }
                }
            }
        }


        private static List<ValidDNS> GetServersForSite(string siteDistinguishedName, LdapConnection ldapConnectionHostDomain)
        {
            if (string.IsNullOrEmpty(siteDistinguishedName)) throw new ArgumentNullException("siteDistinguishedName");

            List<ValidDNS> servers = new List<ValidDNS>();

            try
            {
                string filter = "(&(objectClass=Server))"; //"(&(objectClass=Server)(dnshostname=*))";

                SearchRequest searchRequest = new SearchRequest(string.Format("CN=Servers,{0}", siteDistinguishedName), filter, SearchScope.Subtree, "name", "dNSHostName", "distinguishedName");

                //IMPORTANT!!
                //objectGUID, taken from here is not the same objectGUID for computer object, it is objectGUID for server object
                //and computer principal gets objectGUID for computer
                //so we do not need this, we should use additional code to get it!!!

                SearchResponse searchResponse = ldapConnectionHostDomain.SendRequest(searchRequest) as SearchResponse;

                if (searchResponse != null)
                {
                    foreach (SearchResultEntry entry in searchResponse.Entries)
                    {
                        ValidDNS obj = new ValidDNS();
                        obj.IsValid = true;
                        try
                        {
                            obj.ServerName = entry.Attributes["name"][0].ToString();
                            //LINQ
                            //if (servers.Any(addedServer => addedServer.ServerName == obj.ServerName)) continue;

                            bool found = false;
                            foreach (var addedServer in servers)
                            {
                                if (addedServer.ServerName.Equals(obj.ServerName, StringComparison.InvariantCultureIgnoreCase))
                                {
                                    found = true;
                                    break;
                                }
                            }

                            if (found) continue;


                            obj.DistinguishedName = entry.Attributes["distinguishedName"][0].ToString();

                            try
                            {
                                obj.DnsHostName = entry.Attributes["dNSHostName"][0].ToString();

                                if (string.IsNullOrEmpty(obj.DnsHostName))
                                {
                                    obj.IsValid = false;
                                    obj.Note += "dNSHostName attribute value is null. Probably obsolete domain controller object. ";
                                    continue;
                                }

                                obj.DomainName = GetDomainNameFromFqServerName(obj.DnsHostName);
                                obj.FqServerName = obj.DnsHostName; //_adHelperObject.GetFqServerNameFromNameAndDomain(obj.ServerName, obj.DomainName);
                            }
                            catch (Exception)
                            {
                                obj.IsValid = false;
                                obj.Note += "dNSHostName attribute value is null. Obsolete domain controller object. ";
                                continue;
                            }

                            Exception exx = null;
                            IPAddress ip = ipAddressForAdSiteServer(obj.ServerName, out exx);

                            if (ip == null)
                            {
                                obj.IpAddress = string.Empty;
                                obj.IsValid = false;
                                obj.Error += exx.Message;
                                obj.Note += "IPv4 address cannot be provided. ";
                                continue;
                            }

                            obj.IpAddress = ip.ToString();


                            servers.Add(obj);
                        }
                        catch (Exception ex)
                        {
                            obj.Note += "SearchResultEntry problem. "; // Obsolete domain controller object. 
                            obj.Error += string.Format("message: {0}. ", ex.Message);
                            obj.IsValid = false;
                            
                            Console.ResetColor();
                            Console.WriteLine("Log information is stored in the file: {0}", GlobalData.LogFilePath);
                                                        
                            Logger.LogException(LogLevel.Error, "Loop error:GetServersForSite ", ex);
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                Logger.LogException(LogLevel.Error, "GetServersForSite", ex);
            }

            return servers;
        }


        private static string GetSitePath(string identity)
        {
            int index1 = identity.IndexOf(",CN=Sites", StringComparison.InvariantCulture);
            if (index1 != 0)
            {
                string result = string.Format(identity.Substring(index1 + 1));
                return result;
            }
            return identity;
        }

        private static void GetSitesToCheck(bool winsScan, ActiveDirectorySite currentSite, bool isFullScan, int siteLevel, ref List<ActiveDirectorySite> sitesAll, ref List<ActiveDirectorySiteLink> siteLinksChecked, List<ActiveDirectorySiteLink> adSiteLinksAll)
        {
            List<ActiveDirectorySiteLink> currentSiteLinkList2 = new List<ActiveDirectorySiteLink>();
            try
            {
                //LINQ
                //foreach (Command.ActiveDirectorySiteLink adSiteLink in adSiteLinksAll.Where(t => t.ActiveDirectorySiteArray.Any(s => s.DistinguishedName.Equals(currentSite.DistinguishedName, StringComparison.InvariantCultureIgnoreCase))))
                //{
                //    currentSiteLinkList2.Add(adSiteLink);
                //}
                foreach (ActiveDirectorySiteLink adSiteLink in adSiteLinksAll)
                {
                    foreach (ActiveDirectorySite adSite in adSiteLink.ActiveDirectorySiteArray)
                    {
                        if (adSite.DistinguishedName.Equals(currentSite.DistinguishedName, StringComparison.InvariantCultureIgnoreCase))
                        {
                            currentSiteLinkList2.Add(adSiteLink);
                            break;
                        }
                    }
                }


                List<ActiveDirectorySite> tempSiteList = new List<ActiveDirectorySite>();

                //this second foreach is because of OrderBy, it can happen a site to be in more than one site links, and with this we will choose site link with lower cost
                currentSiteLinkList2.Sort((x, y) => x.Cost.CompareTo(y.Cost));

                //LINQ
                //  foreach (Command.ActiveDirectorySiteLink siteLink in currentSiteLinkList2.OrderBy(t => t.Cost))
                foreach (ActiveDirectorySiteLink siteLink in currentSiteLinkList2)
                {
                    if (string.IsNullOrEmpty(siteLink.Name)) continue;
                    //  //if (siteLink.Name.Equals("DEFAULTIPSITELINK", StringComparison.InvariantCultureIgnoreCase)) continue;

                    //LINQ
                    // if (siteLinksChecked.Any(s => s.Name == siteLink.Name)) continue;

                    bool found = false;
                    foreach (var sl  in siteLinksChecked)
                    {
                        if (sl.Name.Equals(siteLink.Name, StringComparison.InvariantCultureIgnoreCase))
                        {
                            found = true;
                            break;
                        }
                    }
                    if (found) continue;

                    siteLinksChecked.Add(siteLink);

                    foreach (ActiveDirectorySite childSite in siteLink.ActiveDirectorySiteArray)
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

                        ActiveDirectorySite childActiveDirectorySite = new ActiveDirectorySite {SiteLinksCount = sitelikscount, Cost = siteLink.Cost + currentSite.Cost, Name = childSite.Name, SiteLinks = null, Level = siteLevel, DistinguishedName = childSite.DistinguishedName};

                        sitesAll.Add(childActiveDirectorySite);
                        tempSiteList.Add(childActiveDirectorySite);
                    }
                }

                if (!isFullScan)
                {
                    if (winsScan)
                    {
                        if ((siteLevel >= GlobalData.MaximalSiteLevel) || (sitesAll.Count >= GlobalData.MaximumSites)) return;
                    }
                    else
                    {
                        if ((siteLevel >= GlobalData.MaximalSiteLevel) || (sitesAll.Count >= GlobalData.MaximumSites)) return;
                    }
                }


                siteLevel += 1;


                foreach (ActiveDirectorySite adSite in tempSiteList)
                {
                    GetSitesToCheck(winsScan, adSite, isFullScan, siteLevel, ref sitesAll, ref siteLinksChecked, adSiteLinksAll);
                }
            }
            catch (Exception ex)
            {                
                Console.ResetColor();
                Console.WriteLine("Log information is stored in the file: {0}", GlobalData.LogFilePath);
                
                Logger.LogException(LogLevel.Error, "GetSitesToCheck", ex);
            }
        }

        public static string GetDomainNameFromFqServerName(string identity)
        {
            int index1 = identity.IndexOf(".", StringComparison.InvariantCulture);
            if (index1 >= 0)
            {
                string result = string.Format(identity.Substring(index1 + 1));
                return result;
            }
            return string.Empty;
        }

        public static IPAddress ipAddressForAdSiteServer(string serverName, out Exception exx)
        {
            exx = null;

            try
            {
                IPHostEntry entry = Dns.GetHostEntry(serverName);

                IPAddress[] ipv4Addresses = Array.FindAll(entry.AddressList, a => a.AddressFamily == AddressFamily.InterNetwork);

                foreach (var ipv4Address in ipv4Addresses)
                {
                    if (IPAddress.IsLoopback(ipv4Address))
                    {
                        continue;
                    }

                    return ipv4Address;
                }
            }
            catch (Exception ex)
            {
                exx = ex;
            }

            return null;
        }

        public static string GetNameFromDistinguishedName(string identity)
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

        #endregion
    }
}