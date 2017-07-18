//using Synergix.ADCE.Lite.Core;

enum PingReplyMark {
    /// <summary>
    ///     IP Address not available
    /// </summary>
            NOT_AVAILABLE = 0, //  -1000,

    /// <summary>
    ///     pingReplay between 0 and 50
    /// </summary>
            HIGH = GlobalData.RATING_MARK_DOMAIN_D,

    /// <summary>
    ///     pingReplay between 50 and 100
    /// </summary>
            MEDIUM = (int) (GlobalData.RATING_MARK_DOMAIN_D * 0.6f), //30

    /// <summary>
    ///     pingReplay between 100 and 500
    /// </summary>
            LOW = (int) (GlobalData.RATING_MARK_DOMAIN_D * 0.4f), // 20,

    /// <summary>
    ///     pingReplay greather than 500
    /// </summary>
            BAD = (int) (GlobalData.RATING_MARK_DOMAIN_D * 0.2f) //10
};
