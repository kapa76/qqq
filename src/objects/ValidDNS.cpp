    class ValidDNS
    {
         Guid ComputerId ;
         int Id ;
         int ValidDNSListId ;
         bool IsValid ;
         string ServerName ;
         string IpAddress ;
         string SiteName ;
         int Order;
         int Cost ;
         int RatingMark ;
         bool IsOnHostSite ;
         bool IsOnHostDomain ;
         string DomainName ;
         int PingReply ;
         string PingReplyMark;
         string DistinguishedName ;
         string DnsHostName ;
         int SiteLevel ;
         bool IsOnHostParentDomain ;
         string OperatingSystem;
         string OperatingSystemServicePack;
         string Note ;
         string FqServerName ;
         string Error ;
         bool IsWritable ;
         bool IsMemberOfInactiveGroup;
         Guid? ObjectGUID;
         int? SiteLinksCount;
         
         virtual List<PortCheck> portCheck;
         virtual DateTime Date;

        public:

        ValidDNS()
        {
            ComputerId = new Guid("00000000-0000-0000-0000-000000000000");
            Id = 0;
            ValidDNSListId = 0;
            IsValid = false;
            ServerName = "";
            IpAddress = "";
            SiteName = "";
            Order = 0;
            Cost = 0;
            RatingMark = 0;
            IsOnHostSite = false;
            IsOnHostDomain = false;
            DomainName = "";
            PingReply = -1;
            PingReplyMark = "";
            DistinguishedName = "";
            DnsHostName = "";
            SiteLevel = 0;
            IsOnHostParentDomain = false;
            OperatingSystem = "";
            OperatingSystemServicePack = "";
            Note = "";
            FqServerName = "";
            Error = "";
            IsWritable = false;
            IsMemberOfInactiveGroup = false;
            PortCheck = new PortCheck[0]; // new HashSet<PortCheck>();
        }

    };
