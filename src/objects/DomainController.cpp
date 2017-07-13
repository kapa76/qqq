using System;
using System.Collections.Generic;
using System.Text;

namespace Synergix.ADCE.Lite.Objects
{
    public class DomainController
    {
        public Guid Id { get; set; }
        public string Name { get; set; }
        public string DomainName { get; set; }
        public string IPAddress { get; set; }
        public string FullyQualifyedServerName { get; set; }
        public string SiteName { get; set; }
        public int Order { get; set; }
        public bool IsDNSServer { get; set; }        
    }
}
