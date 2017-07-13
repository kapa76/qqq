using System.DirectoryServices.ActiveDirectory;
using System.Net.NetworkInformation;

namespace Synergix.ADCE.Lite.Core
{
    public class GlobalData
    {
        //SITE DRIVEN LOGIC
        public const int RATING_MARK_SITE_S = 800;
        public const int RATING_MARK_DOMAIN_S = 149;
        public const int RATING_MARK_PARENT_DOMAIN_S = 90;
        public const int RATING_MARK_IS_SELF_S = 1;
        //    public const int ratingMarkPingS = 50;


        //DOMAIN DRIVEN LOGIC
        public const int RATING_MARK_SITE_D = 150;
        public const int RATING_MARK_DOMAIN_D = 799;
        public const int RATING_MARK_PARENT_DOMAIN_D = 90;
        public const int RATING_MARK_IS_SELF_D = 1;

        public static string MachineAdSiteDistinguishedName;
        //  public const int ratingMarkPingD = 50;


        public static int RatingMarkSite { get; set; }
        public static int RatingMarkDomain { get; set; }
        public static int RatingMarkParentDomain { get; set; }
        public static int RatingMarkIsSelf { get; set; }
        public static int RatingMarkPing { get; set; }
        public static bool DomainPriority { get; set; }
        public static bool DNSServerOnly { get; set; }
        public static int MaximumDNSServers { get; set; }
        public static int MaximumSites { get; set; }
        public static int MaximalSiteLevel { get; set; }
        

       // public static NetworkInterface NetworkInterface { get; set; }
        //public static string MachineFqName { get; set; }

        //public static Domain HostDomain { get; set; }
        //public static string HostDomainName { get; set; }

        //public static ActiveDirectorySite MachineActiveDirectorySite { get; set; }

        //public static string LogFilePath { get; set; }

        public static void Init()
        {
            RatingMarkSite = 150;
            RatingMarkDomain = 799;
            RatingMarkParentDomain = 90;
            RatingMarkIsSelf = 1;
            RatingMarkPing = 50;
            DomainPriority = true;
            DNSServerOnly = true;
            MaximumDNSServers = 3;
            MaximumSites = 5;
            MaximalSiteLevel = 2;
            //ReadOnly = true;
            //PrimaryDnsAuto = true;
            //AlternateDnsAuto = true;
            //TerciaryDnsAuto = true;
            //PrimaryDnsIp = string.Empty;
            //AlternateDnsIp = string.Empty;
            //TerciaryDnsIp = string.Empty;
            //LogFilePath = "{APPLICATION_DIR}/logs/SYNERGIX-NICE-Lite.log.txt";
        }
    }
}