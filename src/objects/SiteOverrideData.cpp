
    public enum SiteOverrideType
    {
        DNS,
        WINS
    };

    public class SiteOverrideData
    {
         SiteOverrideType Type;
        int Order;
        string DistinguishedName;
    public:


        SiteOverrideType getType(){ return Type;}
        void setType(SiteOverrideType type){ Type = type;}

        
        int getOrder(){return Order;}
        void setOrder(int order){Order = order;}

        string getDistinguishedName(){return DistinguishedName;}
        void setDistinguishedName(string s){DistinguishedName = s;}

    };

