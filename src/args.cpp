
class Arguments {

public:
    List <string> AddParameter; //{get;}
    Dictionary <string, string> Parameters{get;}

public

    Arguments(string[] args) {
        int count = 0;

        AddParameter = new List<string>();
        Parameters = new Dictionary<string, string>(StringComparer.InvariantCultureIgnoreCase);

        Regex
        spliter = new Regex(@"^-{1,2}|^/|=|:", RegexOptions.IgnoreCase | RegexOptions.Compiled);

        int splitcount = 3;
        int eqaul = 0;
        int ntequal = 0;
        string parameter = string.Empty;

        foreach(string
        input
                in
        args)
        {
            string txt = input.ToLower();
            int count1 = new Regex("=|:").Matches(input).Count;

            if (Parameters.ContainsKey("InvalidKey") | Parameters.ContainsKey("DuplicateKey")) break;

            if (input.StartsWith("-") || input.ToLower() == "/?") {
                count = 0;
                if (parameter.StartsWith("readonly") || parameter.StartsWith("update") ||
                    parameter == "domainPriority" || parameter == "dnsonly" || parameter == "network" ||
                    parameter == "pridns" || parameter == "altdns" || parameter == "terdns") {
                    string newTxt = parameter;
                    parameter = "InvalidKey";
                    Parameters.Add(parameter, newTxt);
                    parameter = string.Empty;
                    break;
                }
            } else if (txt.ToLower() == "readonly") {
                parameter = "readonly";
            } else if (txt.ToLower() == "update") {
                parameter = "update";
            }

            string[]
            parts;

            if (parameter.ToLower() == "network") {
                parts = new
                [] { input };
            } else {
                if (count1 > 1) {
                    splitcount -= 1;
                }

                parts = spliter.Split(input, splitcount);
                splitcount = 3;
            }

            if (count > 0) {
                if (txt == "/network" || txt.StartsWith("/network")) {
                    count--;
                } else {
                    if (txt.Contains("=")) {
                        eqaul++;
                    } else {
                        ntequal++;
                    }

                    if (eqaul == ntequal) {
                        parameter = "InvalidAttr";
                        Parameters.Add(parameter, "InvalidVal");
                        //countparam++;
                        parameter = string.Empty;
                        break;
                    }
                    if (eqaul > 0 & ntequal > 0) {
                        parameter = "InvalidAttr";
                        Parameters.Add(parameter, "InvalidVal");
                        //countparam++;
                        parameter = string.Empty;
                        break;
                    }
                    if (string.IsNullOrEmpty(parameter) & parts.Length == 1) {
                        parameter = parts[0];
                        AddParameter.Add(parameter);
                        parameter = string.Empty;
                        continue;
                    }
                }
            }

            if (Parameters.ContainsKey(parameter)) {
                Parameters.Add("DuplicateKey", parameter);
                parameter = string.Empty;
                break;
            }

            switch (parts.Length) {
                case 1:
                    if (parameter != string.Empty) {
                        if (!Parameters.ContainsKey(parameter)) {
                            Parameters.Add(parameter, parts[0]);
                        }

                        parameter = string.Empty;
                    } else {
                        parameter = "InvalidKey";
                        Parameters.Add(parameter, txt);
                        parameter = string.Empty;
                    }

                    break;

                case 2:

                    if (parameter != string.Empty) {
                        if (!Parameters.ContainsKey(parameter)) {
                            Parameters.Add(parameter, "true");
                        }
                    }

                    if (parts[1] == "attr") {
                        parameter = parts[1];

                        if (Parameters.ContainsKey(parameter)) {
                            Parameters.Add("DuplicateKey", parameter);
                            parameter = string.Empty;
                            break;
                        }

                        Parameters.Add(parameter, "true");
                        parameter = string.Empty;
                        count++;

                        break;
                    }

                    if (parts[0] == "") {
                        parameter = parts[1];
                    } else {
                        parameter = parts[0];
                        Parameters.Add(parameter, parts[1]);
                        parameter = string.Empty;
                    }
                    break;

                case 3:

                    if (parameter != string.Empty) {
                        if (!Parameters.ContainsKey(parameter)) {
                            Parameters.Add(parameter, "true");
                        }
                    }

                    parameter = string.IsNullOrEmpty(parts[1]) ? parts[0] : parts[1];

                    if (!Parameters.ContainsKey(parameter)) {
                        Parameters.Add(parameter, parts[2]);
                    }

                    parameter = string.Empty;
                    break;
            }
        }

        if (Parameters.ContainsKey(parameter)) {
            Parameters.Add("DuplicateKey", parameter);
        } else {
            if (parameter != string.Empty) {
                if (!Parameters.ContainsKey(parameter))
                    Parameters.Add(parameter, "true");
                else
                    Parameters.Add(parameter, "true");
            }
        }
    }

public string this
    [string Param
    ]
    {
        get {return (Parameters[Param]);}
        set {Param = value;}
    }

public

    bool IsArgumentSpecifed(string arg) {
        return Parameters.ContainsKey(arg);
    }
}
