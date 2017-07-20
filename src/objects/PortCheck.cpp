class PortCheck
    {
         Guid ComputerId ;
         int Id ;
         int ValidDNSId;
         string IpType;
         int PortNumber ;
         bool Value;
         string Note;

        public:

        PortCheck()
        {
            ComputerId = new Guid("00000000-0000-0000-0000-000000000000");
            ValidDNSId = 0;
            IpType = "TCP";
            PortNumber = 0;
            Value = false;
        }

        public PortCheck(Enum IpType, int PortNumber, bool Value)
        {
            ComputerId = new Guid("00000000-0000-0000-0000-000000000000");
            ValidDNSId = 0;
            this.IpType = IpType.ToString();
            this.PortNumber = PortNumber;
            this.Value = Value;
        }

         Guid getComputerId();
         void setGuid(string a);

         int Id { get; set; }
         int ValidDNSId { get; set; }
         string IpType { get; set; }
         int PortNumber { get; set; }
         bool Value { get; set; }
         string Note { get; set; }

        public virtual ValidDNS ValidDNS { get; set; }
    };
