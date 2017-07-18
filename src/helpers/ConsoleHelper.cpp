class ConsoleHelper {
public:

    static void SmartWriteLine(string label, string msg) {
        string tmpMsg = msg;

        while (!string.IsNullOrEmpty(tmpMsg)) {
            tmpMsg = tmpMsg.TrimStart();
            Console.Write("{0}\t", label);
            string s = SmartTrim(tmpMsg, Console.BufferWidth - Console.CursorLeft - 2);
            Console.WriteLine(s);
            tmpMsg = tmpMsg.Remove(0, s.Length);
        }
    }

public:

    static string SmartTrim(string text, int length) {
        if (string.IsNullOrEmpty(text)) {
            throw new ArgumentNullException(nameof(text));
        }
        if (text.Length <= length) {
            return text;
        }
        int lastSpaceBeforeMax = text.LastIndexOf(' ', length);

        if (lastSpaceBeforeMax == -1) {
            return text;
        }
        return text.Substring(0, lastSpaceBeforeMax);
    }
};
