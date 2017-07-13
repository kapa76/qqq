using System;
using System.DirectoryServices.ActiveDirectory;
using System.Net.NetworkInformation;
using NLog;
using Synergix.ADCE.Lite.Objects;
using Synergix.ADCE.Lite.Providers;
using ActiveDirectorySite = Synergix.ADCE.Lite.Objects.ActiveDirectorySite;

namespace Synergix.ADCE.Lite.Commands
{
    public class CommandContext
    {
        private readonly Options _Options;        
        private readonly Logger _Logger;

        public CommandContext(Options options)
        {
            _Options = options;
            _Logger = LogManager.GetCurrentClassLogger();
        }

        public string DomainName { get; set; }

        public string ParentDomainName { get; set; }

        public bool IsForestRoot { get; set; }

        public string ComputerFQNName { get; set; }

        public ActiveDirectorySite ComputerActiveDirectorySite { get; set; }

        public NetworkInterface NetworkInterface { get; set; }

        public bool Initialize()
        {
            try
            {
                NetworkInterfaceProvider interfaceProvider = new NetworkInterfaceProvider(_Options);

                NetworkInterface = interfaceProvider.GetDefaultInteface();

                if (NetworkInterface == null)
                {
                    var message = string.IsNullOrEmpty(_Options.NetworkInterfaceName)
                                      ? "This computer has multiple NIC configured. Since you did not provide network interface name, this command will not run"
                                      : $"Network Connection [{_Options.NetworkInterfaceName}] was not found. Please provide valid network connection name.";

                    Console.ForegroundColor = ConsoleColor.Red;
                    Console.WriteLine("Error:\t{0}", message);
                    Console.ResetColor();

                    _Logger.Error(message);

                    return false;
                }
            }
            catch (Exception ex)
            {
                _Logger.LogException(LogLevel.Error, "Cannot find default network adapter", ex);
                return false;
            }

            try
            {
                using (var domain = Domain.GetComputerDomain())
                {
                    DomainName = domain.Name;

                    using (var parentDomain = domain.Parent)
                    {
                        if (parentDomain == null)
                        {
                            using (var forest = domain.Forest)
                            {
                                using (var rootDomain = forest.RootDomain)
                                {
                                    if (!string.Equals(DomainName, rootDomain.Name, StringComparison.InvariantCultureIgnoreCase))
                                    {
                                        ParentDomainName = rootDomain.Name;
                                    }
                                }
                            }
                        }
                        else
                        {
                            ParentDomainName = parentDomain.Name;
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                Console.ForegroundColor = ConsoleColor.Yellow;
                Console.WriteLine();
                Console.WriteLine("Error: Computer domain is not in reach!");
                Console.WriteLine("Check network connectivity and ensure at least one valid DNS Server IP address is configured in IPv4 properties.");
                Console.WriteLine();
                Console.ResetColor();

                _Logger.Error("Computer domain is not in reach");
                _Logger.Error("Check network connectivity and ensure at least one valid DNS Server IP address is configured in IPv4 properties.");
                _Logger.LogException(LogLevel.Error, "Constructor", ex);

                return false;
            }

            try
            {
                ComputerFQNName = $"{Environment.MachineName.ToUpperInvariant()}.{DomainName}";
            }
            catch (Exception ex)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("Error: \tCannot read computer name from DNS");
                Console.ResetColor();
                Console.WriteLine("Log information is stored in the file: {0}", _Options.LogFilePath);

                _Logger.LogException(LogLevel.Error, "Cannot read computer name from DNS", ex);
                return false;
            }

            try
            {
                using (var site = System.DirectoryServices.ActiveDirectory.ActiveDirectorySite.GetComputerSite())
                {
                    ComputerActiveDirectorySite = new Objects.ActiveDirectorySite
                                                  {
                                                      SiteLinksCount = site.SiteLinks.Count,
                                                      Cost = 0,
                                                      Name = site.Name,
                                                      SiteLinks = site.SiteLinks,
                                                      Level = 0
                                                  };

                    using (var de = site.GetDirectoryEntry())
                    {
                        var property = de.Properties["distinguishedname"];

                        if (property != null)
                        {
                            if (property.Value != null)
                            {
                                ComputerActiveDirectorySite.DistinguishedName = property.Value.ToString();
                            }
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("Error: \tCannot contact computer AD site!");
                Console.ResetColor();
                Console.WriteLine("Log information is stored in the file: {0}", _Options.LogFilePath);

                _Logger.LogException(LogLevel.Error, "Cannot contact computer AD site", ex);
                return false;
            }

            return true;
        }
    }
}