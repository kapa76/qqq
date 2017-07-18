
class GlobalData {
    //SITE DRIVEN LOGIC
public:
    const int RATING_MARK_SITE_S = 800;
    const int RATING_MARK_DOMAIN_S = 149;
    const int RATING_MARK_PARENT_DOMAIN_S = 90;
    const int RATING_MARK_IS_SELF_S = 1;
    //    public const int ratingMarkPingS = 50;


    //DOMAIN DRIVEN LOGIC
    const int RATING_MARK_SITE_D = 150;
    const int RATING_MARK_DOMAIN_D = 799;
    const int RATING_MARK_PARENT_DOMAIN_D = 90;
    const int RATING_MARK_IS_SELF_D = 1;

    static string MachineAdSiteDistinguishedName;
    //  public const int ratingMarkPingD = 50;


    static int RatingMarkSite{get; set;}
    static int RatingMarkDomain{get; set;}
    static int RatingMarkParentDomain{get; set;}
    static int RatingMarkIsSelf{get; set;}
    static int RatingMarkPing{get; set;}
    static bool DomainPriority{get; set;}
    static bool DNSServerOnly{get; set;}
    static int MaximumDNSServers{get; set;}
    static int MaximumSites{get; set;}
    static int MaximalSiteLevel{get; set;}

    // public static NetworkInterface NetworkInterface { get; set; }
    //public static string MachineFqName { get; set; }

    //public static Domain HostDomain { get; set; }
    //public static string HostDomainName { get; set; }

    //public static ActiveDirectorySite MachineActiveDirectorySite { get; set; }

    //public static string LogFilePath { get; set; }

    static void Init() {
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
};
