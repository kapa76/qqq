namespace Synergix.ADCE.Lite.Objects
{
    public class SiteOverrideData
    {
        public SiteOverrideType Type { get; set; }

        public string DistinguishedName { get; set; }

        public int Order { get; set; }
    }

    public enum SiteOverrideType
    {
        DNS,
        WINS
    }
}