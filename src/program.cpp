using System;
using System.Collections.Generic;
using System.Reflection;
using System.Text;
using NLog;
using Synergix.ADCE.Lite.Commands;
using Synergix.ADCE.Lite.Core;
using Synergix.ADCE.Lite.Objects;

namespace Synergix.ADCE.Lite
{
    internal class Program
    {
        private static readonly Logger Logger = LogManager.GetCurrentClassLogger();
        private static Options _options;

        private static void Main(string[] args)
        {
            GlobalData.Init();

            DisplayDesc();

            _options = new Options(args);
            string parseError = _options.Parse();

            if (!string.IsNullOrEmpty(parseError))
            {
                DisplayUsage();

                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("Error:");
                Console.WriteLine("\t{0}", parseError);
                Console.ResetColor();

                return;
            }

            if (_options.ExecutionOption == ExecutionOption.HELP)
            {
                DisplayUsage();
            }
            else
            {
                InitLog(args);

                CommandContext context = new CommandContext(_options);

                if (!context.Initialize())
                {
                    return;
                }

                if (_options.ReadOnlyMode)
                {
                    Console.ResetColor();
                    Console.WriteLine();
                    Console.ForegroundColor = ConsoleColor.Yellow;
                    Console.WriteLine("Information:\tProgram started in READONLY mode. No changes will be made.");
                    Console.ResetColor();

                    Logger.Info("Program started in READONLY mode. No changes will be made.");                    
                }
                else
                {
                    Console.ResetColor();
                    Console.WriteLine();
                    Console.ForegroundColor = ConsoleColor.Yellow;
                    Console.WriteLine("Information:\tProgram started in UPDATE mode. Optimal configuration will be applied.");
                    Console.ResetColor();

                    Logger.Warn("Program started in UPDATE mode. Optimal configuration will be applied.");
                }

                switch (_options.ExecutionOption)
                {                    
                    case ExecutionOption.DNS:
                        
                        RemediateDNSServersCommand command = new RemediateDNSServersCommand(_options, context);

                        if (command.Execute())
                        {
                            DisplaySuccess();
                        }
                        else
                        {
                            DisplayError();
                        }

                        break;
                    case ExecutionOption.WINS:

                        RemediateWINSServersCommand winsCommand = new RemediateWINSServersCommand(_options, context);

                        if (winsCommand.Execute())
                        {
                            DisplaySuccess();
                        }
                        else
                        {
                            DisplayError();
                        }

                        break;
                }
            }
            
            Console.WriteLine();
            Console.ResetColor();
        }


        private static void InitLog(string[] args)
        {
            var version = Assembly.GetEntryAssembly().GetName().Version.ToString();

            Logger.Info("========================================================");
            Logger.Info("SYNERGIX ADCE Lite");
            Logger.Info("(c) 2016 - Synergix, Inc.");
            Logger.Info("www.synergix.com");
            Logger.Info("Version {0}", version);

            StringBuilder sb = new StringBuilder();
            sb.Append("Program started; running: SYNERGIX-ADCE-Lite.exe");

            foreach (var s in args)
            {
                sb.Append($" {s}");
            }

            Logger.Info(sb.ToString);
        }

        public static void DisplayDesc()
        {
            Console.WriteLine();
            var version = Assembly.GetEntryAssembly().GetName().Version.ToString();

            Console.WriteLine("SYNERGIX ADCE Lite");
            Console.WriteLine("(c) 2016 - Synergix, Inc.");
            Console.WriteLine("www.synergix.com");
            Console.WriteLine("Version {0}", version);
            Console.WriteLine();
            Console.WriteLine();
            Console.WriteLine("License:");
            Console.WriteLine();
            Console.WriteLine("\tContact software vendor for commercial use license requirements.");
            Console.ForegroundColor = ConsoleColor.Yellow;
            Console.WriteLine("\tTo learn more about the full version of the product, visit http://www.synergix.com.");
            Console.ResetColor();
            Console.WriteLine();
            Console.WriteLine();
            Console.WriteLine("Description:");
            Console.WriteLine();
            Console.WriteLine("\tThe software allows Windows Server Administrators to update DNS Client");
            Console.WriteLine("\tconfiguration by discovering optimal DNS Servers in Active Directory domain.");
            Console.WriteLine();
            Console.WriteLine("\tThe software will remediate primary, alternate and tertiary DNS Server IP Addresses.");
            Console.WriteLine("\tIf more than 3 DNS Server IP Addresses are already configured (valid or invalid),");
            Console.WriteLine("\tthey will be wiped out");
            Console.WriteLine();
        }

        public static void DisplayUsage()
        {
            Console.ResetColor();
            Console.WriteLine();
            Console.WriteLine("Syntax:");
            Console.WriteLine();
            Console.Write("\t");
            Console.WriteLine(@"SYNERGIX-ADCE-Lite.exe [readonly | update] -[d|dns | w|wins]");
            Console.Write("\t");
            Console.WriteLine(@"-[dns1 | wins1]=[auto | IPv4 Address]");
            Console.Write("\t");
            Console.WriteLine(@"-[dns2 | wins2]=[auto | IPv4 Address]");
            Console.Write("\t");
            Console.WriteLine("-[dns3]=[auto | IPv4 Address | none]");
            Console.Write("\t");
            Console.WriteLine("-network=Network_Connection_Name");
            Console.Write("\t");
            Console.WriteLine("-f|force");
            Console.WriteLine();
            Console.WriteLine();
            Console.WriteLine("Parameters:");
            Console.WriteLine();
            Console.Write("\t");
            Console.WriteLine(@"readonly");
            Console.WriteLine("\t\tReadonly mode. Discover AD Site Topology and report optimal DNS or WINS Servers");
            Console.WriteLine("\t\tEither readonly or update parameter may be used");
            Console.WriteLine();
            Console.Write("\t");
            Console.WriteLine(@"update");
            Console.WriteLine("\t\tUpdate mode. Application performs remediation and updates the DNS Client configuration.");
            Console.ForegroundColor = ConsoleColor.Red;
            Console.WriteLine("\t\tNote: The requested UPDATE operation always requires elevation (Run as administrator).");
            Console.ResetColor();

            Console.WriteLine();
            Console.Write("\t");
            Console.WriteLine(@"-d|dns");
            Console.Write("\t");
            Console.WriteLine("\tRequired if -w|wins is not specified. Discovers and updates DNS Servers");

            Console.WriteLine();
            Console.Write("\t");
            Console.WriteLine(@"-w|wins");
            Console.Write("\t");
            Console.WriteLine("\tRequired if -d|dns is not specified. Discovers and updates WINS Servers");

            Console.WriteLine();
            Console.Write("\t");
            Console.WriteLine(@"-network=Network_Connection_Name");
            Console.Write("\t");
            Console.WriteLine("\tOptional. If network adapter is not specified default network adapter is used.");
            Console.Write("\t");
            Console.WriteLine("\tIf Network Connection Name contains spaces, enclose in double quotes");

            Console.WriteLine();
            Console.Write("\t");
            Console.WriteLine(@"-dns1=[auto | IPv4 Address]");
            Console.Write("\t");
            Console.WriteLine("\tOptional. If skipped, default is set to Auto");
            Console.Write("\t");
            Console.WriteLine("\tIf DNS Service service is running on computer, primary DNS will always be local IP address");

            Console.WriteLine();
            Console.Write("\t");
            Console.WriteLine(@"-dns2=[auto | IPv4 Address]");
            Console.Write("\t");
            Console.WriteLine("\tOptional. If skipped, default is set to Auto");

            Console.WriteLine();
            Console.Write("\t");
            Console.WriteLine(@"-dns3=[auto | IPv4 Address | none]");
            Console.Write("\t");
            Console.WriteLine("\tOptional. If skipped, default is set to Auto");

            Console.WriteLine();
            Console.Write("\t");
            Console.WriteLine(@"-f|force");
            Console.Write("\t");
            Console.WriteLine("\tOptional. If specified the current DNS or WINS configuration will be overridden; othewise the");
            Console.Write("\t");
            Console.WriteLine("\tcurrent DNS or WINS configuration will be overriden only if the servers are not from the optimal sites");

            Console.WriteLine();

            Console.WriteLine();
            Console.WriteLine(@"Examples:");
            Console.WriteLine();
            Console.WriteLine("\tSYNERGIX-ADCE-Lite readonly -dns");
            Console.WriteLine();
            Console.WriteLine("\tSYNERGIX-ADCE-Lite update -dns");
            Console.WriteLine();
            Console.WriteLine("\tSYNERGIX-ADCE-Lite update -dns -dns1=auto -dns2=auto -dns3=8.8.8.8");
            Console.WriteLine();
            Console.WriteLine("\tSYNERGIX-ADCE-Lite update -dns -dns1=10.1.1.1 -dns2=10.1.1.2 -dns3=10.1.1.3 -network=Ethernet");
            Console.WriteLine();
            Console.WriteLine("\tSYNERGIX-ADCE-Lite update -wins");
            Console.WriteLine();
        }

        public static void DisplaySuccess()
        {
            Console.ResetColor();
            Console.WriteLine("Information:\tProgram completed successfully.");
            Console.WriteLine();
            Console.WriteLine("For more information, please visit http://www.synergix.com/products/network-infrastructure-client-extensions/");
            Console.WriteLine("For support related questions, please write to support@synergix.com");
        }

        public static void DisplayError()
        {
            Console.ForegroundColor = ConsoleColor.Red;
            Console.WriteLine("Error: \tApplication completed with error!");
        }

        public static List<string> SplitStringByLineLentgh(int partLength, string sentence)
        {
            var words = sentence.Split(' ');
            var parts = new Dictionary<int, string>();
            var part = string.Empty;
            var partCounter = 0;

            foreach (var word in words)
            {
                if (part.Length + word.Length < partLength)
                {
                    part += string.IsNullOrEmpty(part) ? word : " " + word;
                }
                else
                {
                    parts.Add(partCounter, part);
                    part = word;
                    partCounter++;
                }
            }
            parts.Add(partCounter, part);

            var result = new List<string>();

            for (var i = 0; i < parts.Count; i++)
            {
                result.Add(parts[i]);
                if ((parts.Count == 1) || (i == parts.Count - 1))
                {
                    //last or only one , not needed line brake
                }
            }
            return result;
        }
    }
}