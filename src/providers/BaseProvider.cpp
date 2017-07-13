using System;
using System.Collections.Generic;
using System.Text;
using Synergix.ADCE.Lite.Objects;

namespace Synergix.ADCE.Lite.Providers
{
    public class BaseProvider
    {
        protected Options Options { get; }

        public BaseProvider(Options options)
        {
            Options = options;
        }
    }
}
