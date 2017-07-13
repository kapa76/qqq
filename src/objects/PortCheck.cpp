using System;

namespace Synergix.ADCE.Lite.Objects
{
    public class PortCheck
    {
        public PortCheck()
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
            //  this.ComputerId = ComputerId;
        }

        public Guid ComputerId { get; set; }
        public int Id { get; set; }
        public int ValidDNSId { get; set; }
        public string IpType { get; set; }
        public int PortNumber { get; set; }
        public bool Value { get; set; }
        public string Note { get; set; }

        public virtual ValidDNS ValidDNS { get; set; }
    }
}