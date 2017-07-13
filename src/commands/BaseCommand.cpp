using System;
using NLog;
using Synergix.ADCE.Lite.Helpers;
using Synergix.ADCE.Lite.Objects;

namespace Synergix.ADCE.Lite.Commands
{
    public class BaseCommand
    {
        public BaseCommand(Options options, CommandContext context)
        {
            Options = options;
            Context = context;
        }

        protected Options Options { get; }

        protected CommandContext Context { get; }

        protected Logger Logger { get; set; }

        public void WriteInformation(string msg)
        {
            Console.ResetColor();
            Console.ForegroundColor = ConsoleColor.Yellow;
            ConsoleHelper.SmartWriteLine("Information:", msg);
            Console.ResetColor();
            Logger.Info(msg);
        }

        public void WriteError(string msg)
        {
            Console.ResetColor();
            Console.ForegroundColor = ConsoleColor.Red;
            ConsoleHelper.SmartWriteLine("Warning:", msg);
            Console.ResetColor();
            Logger.Warn(msg);
        }

        public void WriteSuccess(string msg)
        {
            Console.ResetColor();
            Console.ForegroundColor = ConsoleColor.Green;
            ConsoleHelper.SmartWriteLine("Information:", msg);
            Console.ResetColor();
            Logger.Info(msg);
        }
    }
}