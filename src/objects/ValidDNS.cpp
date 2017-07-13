using System;
using System.Collections.Generic;

namespace Synergix.ADCE.Lite.Objects
{
    public class ValidDNS
    {
        public ValidDNS()
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

        public Guid ComputerId { get; set; }
        public int Id { get; set; }
        public int ValidDNSListId { get; set; }
        public bool IsValid { get; set; }
        public string ServerName { get; set; }
        public string IpAddress { get; set; }
        public string SiteName { get; set; }
        public int Order { get; set; }
        public int Cost { get; set; }
        public int RatingMark { get; set; }
        public bool IsOnHostSite { get; set; }
        public bool IsOnHostDomain { get; set; }
        public string DomainName { get; set; }
        public int PingReply { get; set; }
        public string PingReplyMark { get; set; }
        public string DistinguishedName { get; set; }
        public string DnsHostName { get; set; }
        public int SiteLevel { get; set; }
        public bool IsOnHostParentDomain { get; set; }
        public string OperatingSystem { get; set; }
        public string OperatingSystemServicePack { get; set; }
        public string Note { get; set; }
        public string FqServerName { get; set; }
        public string Error { get; set; }
        public bool IsWritable { get; set; }
        public bool IsMemberOfInactiveGroup { get; set; }
        public Guid? ObjectGUID { get; set; }
        public int? SiteLinksCount { get; set; }
        public virtual ICollection<PortCheck> PortCheck { get; set; }
        public virtual DateTime Date { get; set; }
    }
}