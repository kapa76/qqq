using System.Collections.Generic;

namespace Synergix.ADCE.Lite.Objects
{
    public class ActiveDirectorySiteLink
    {
        public string Name { get; set; }

        public string DistinguishedName { get; set; }

        public int Cost { get; set; }

        public List<ActiveDirectorySite> ActiveDirectorySiteArray { get; set; }        
    }
}