
class ActiveDirectorySite {
        public:
         string Name { get; set; }

         string DistinguishedName { get; set; }

         int Cost { get; set; }

         int Level { get; set; }

         int SiteLinksCount { get; set; }

         ReadOnlySiteLinkCollection SiteLinks { get; set; }
    };
		