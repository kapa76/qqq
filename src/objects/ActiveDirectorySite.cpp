using System.DirectoryServices.ActiveDirectory;

namespace Synergix.ADCE.Lite.Objects
{
    public class ActiveDirectorySite
    {
        public string Name { get; set; }

        public string DistinguishedName { get; set; }

        public int Cost { get; set; }

        public int Level { get; set; }

        public int SiteLinksCount { get; set; }

        public ReadOnlySiteLinkCollection SiteLinks { get; set; }
    }
}