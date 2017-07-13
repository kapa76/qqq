using System;
using System.Collections.Generic;
using System.IO;
using System.Net;
using System.Reflection;
using Mono.Options;
using NLog;

namespace Synergix.ADCE.Lite.Objects
{
    public class Options
    {
        private readonly string[] _Args;
        private readonly Logger _Logger;
        private bool _ShowHelp;
        private bool _UpdateDNS;
        private bool _UpdateWINS;
        private int? _CurrentSiteNumberOfServers;

        public Options(string[] args)
        {
            _Logger = LogManager.GetCurrentClassLogger();
            _Args = args;

            string path = Assembly.GetExecutingAssembly().CodeBase;

            if (string.IsNullOrEmpty(path))
            {
                LogFilePath = "{APPLICATION_DIR}\\logs\\SYNERGIX-ADCE-Lite.log.txt";
            }
            else
            {
                var dir = Path.GetDirectoryName(path);
                LogFilePath = string.IsNullOrEmpty(dir)
                                  ? "{APPLICATION_DIR}\\logs\\SYNERGIX-ADCE-Lite.log.txt"
                                  : Path.Combine(dir, "logs\\SYNERGIX-ADCE-Lite.log.txt");
            }
        }

        public bool ReadOnlyMode { get; set; }

        public bool Force { get; set; }

        public string NetworkInterfaceName { get; set; }

        public string DNSServer1 { get; set; }
        public string DNSServer2 { get; set; }
        public string DNSServer3 { get; set; }

        public string WINSServer1 { get; set; }

        public string WINSServer2 { get; set; }

        public int CurrentSiteNumberOfServers { get; set; }

        public ExecutionOption ExecutionOption { get; set; }

        public string LogFilePath { get; set; }

        public string Parse()
        {
            string error;

            OptionSet p = new OptionSet
                          {
                              {
                                  "h|?|help", "Display Help", delegate(string x)
                                                              {
                                                                  if (!string.IsNullOrEmpty(x))
                                                                  {
                                                                      _ShowHelp = true;
                                                                  }
                                                              }
                              },
                              {
                                  "d|dns", "Update DNS", delegate(string x)
                                                         {
                                                             if (!string.IsNullOrEmpty(x))
                                                             {
                                                                 _UpdateDNS = true;
                                                             }
                                                         }
                              },
                              {
                                  "w|wins", "Update WINS", delegate(string x)
                                                           {
                                                               if (!string.IsNullOrEmpty(x))
                                                               {
                                                                   _UpdateWINS = true;
                                                               }
                                                           }
                              },
                              {
                                  "n|network=", "Network Adapter Name", delegate(string x)
                                                                        {
                                                                            if (!string.IsNullOrEmpty(x))
                                                                            {
                                                                                NetworkInterfaceName = x;
                                                                            }
                                                                        }
                              },
                              {
                                  "f|force", "Force update", delegate(string x)
                                                                        {
                                                                            if (!string.IsNullOrEmpty(x))
                                                                            {
                                                                                Force = true;
                                                                            }
                                                                        }
                              },
                              {
                                  "dns1=", "DNS1", delegate(string x)
                                                   {
                                                       if (!string.IsNullOrEmpty(x))
                                                       {
                                                           DNSServer1 = x;
                                                       }
                                                   }
                              },
                              {
                                  "dns2=", "DNS2", delegate(string x)
                                                   {
                                                       if (!string.IsNullOrEmpty(x))
                                                       {
                                                           DNSServer2 = x;
                                                       }
                                                   }
                              },
                              {
                                  "dns3=", "DNS3", delegate(string x)
                                                   {
                                                       if (!string.IsNullOrEmpty(x))
                                                       {
                                                           DNSServer3 = x;
                                                       }
                                                   }
                              },
                              {
                                  "wins1=", "WINS1", delegate(string x)
                                                     {
                                                         if (!string.IsNullOrEmpty(x))
                                                         {
                                                             WINSServer1 = x;
                                                         }
                                                     }
                              },
                              {
                                  "wins2=", "WINS2", delegate(string x)
                                                     {
                                                         if (!string.IsNullOrEmpty(x))
                                                         {
                                                             WINSServer2 = x;
                                                         }
                                                     }
                              },
                              {
                                  "site1=", "Current Site Number of Servers", delegate(string x)
                                                                              {
                                                                                  if (!string.IsNullOrEmpty(x))
                                                                                  {
                                                                                      int res;
                                                                                      if (int.TryParse(x, out res))
                                                                                      {
                                                                                          _CurrentSiteNumberOfServers = res;
                                                                                      }
                                                                                  }
                                                                              }
                              }
                          };

            try
            {
                List<string> extra = p.Parse(_Args);

                if (extra.Count != 1)
                {
                    _ShowHelp = true;
                    error = extra.Count > 1 ? "You have provided too many parametars" : "READONLY or UPDATE parameter must be specified";
                }
                else
                {
                    ReadOnlyMode = !string.Equals("update", extra[0], StringComparison.InvariantCultureIgnoreCase);

                    error = Validate();

                    if (!string.IsNullOrEmpty(error))
                    {
                        _ShowHelp = true;
                    }
                }
            }
            catch (OptionException e)
            {
                _Logger.LogException(LogLevel.Error, "Unable to parse input parametars", e);
                _ShowHelp = true;
                error = e.Message;
            }
            catch (Exception ex)
            {
                _Logger.LogException(LogLevel.Error, "Unable to parse input parametars", ex);
                _ShowHelp = true;
                error = "General error ocured during parameters parsing. Please see usage for details";
            }

            if (_ShowHelp)
            {
                error = string.Empty;
                ExecutionOption = ExecutionOption.HELP;
            }
            else
            {
                //Becouse we validate before this check we are certian that they are exclusive and only one is true
                ExecutionOption = _UpdateDNS ? ExecutionOption.DNS : ExecutionOption.WINS;
            }

            CurrentSiteNumberOfServers = _CurrentSiteNumberOfServers ?? 1;

            return error;
        }

        private string Validate()
        {
            if (_UpdateDNS && _UpdateWINS)
            {
                return "DNS and WINS parameters cannot be specified at the same time";
            }

            if (_UpdateDNS == false && _UpdateWINS == false)
            {
                return "DNS or WINS parameter must be specified";
            }

            if (string.Equals("none", DNSServer1, StringComparison.InvariantCultureIgnoreCase))
            {
                return "DNS1 can't be none";
            }

            if (string.Equals("none", DNSServer2, StringComparison.InvariantCultureIgnoreCase))
            {
                return "DNS2 can't be none";
            }

            if (string.Equals("auto", DNSServer1, StringComparison.InvariantCultureIgnoreCase))
            {
                DNSServer1 = string.Empty;
            }
            
            if (!string.IsNullOrEmpty(DNSServer1))
            {
                if (!IsIPAddress(DNSServer1))
                {
                    return "You have provided invalid IP Address for DNS1";
                }
            }

            if (string.Equals("auto", DNSServer2, StringComparison.InvariantCultureIgnoreCase))
            {
                DNSServer2 = string.Empty;
            }

            if (!string.IsNullOrEmpty(DNSServer2))
            {
                if (!IsIPAddress(DNSServer2))
                {
                    return "You have provided invalid IP Address for DNS2";
                }
            }

            if (string.Equals("auto", DNSServer3, StringComparison.InvariantCultureIgnoreCase))
            {
                DNSServer3 = string.Empty;
            }

            if (!string.Equals("none", DNSServer3, StringComparison.InvariantCultureIgnoreCase))
            {
                if (!string.IsNullOrEmpty(DNSServer3))
                {
                    if (!IsIPAddress(DNSServer3))
                    {
                        return "You have provided invalid IP Address for DNS3";
                    }
                }
            }
            
            if (string.Equals("auto", WINSServer1, StringComparison.InvariantCultureIgnoreCase))
            {
                WINSServer1 = string.Empty;
            }

            if (!string.IsNullOrEmpty(WINSServer1))
            {
                if (!IsIPAddress(WINSServer1))
                {
                    return "You have provided invalid IP Address for WINS1";
                }
            }

            if (string.Equals("auto", WINSServer2, StringComparison.InvariantCultureIgnoreCase))
            {
                WINSServer2 = string.Empty;
            }

            if (!string.IsNullOrEmpty(WINSServer2))
            {
                if (!IsIPAddress(WINSServer2))
                {
                    return "You have provided invalid IP Address for WINS2";
                }
            }

            return string.Empty;
        }

        private bool IsIPAddress(string ip)
        {
            IPAddress parse;

            if (IPAddress.TryParse(ip, out parse))
            {
                return true;
            }

            return false;
        }
    }
}